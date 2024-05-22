#include "preferences/dialog/dlgprefwaveform.h"

#include "library/dao/analysisdao.h"
#include "library/library.h"
#ifdef __APPLE__
#include "util/cmdlineargs.h"
#endif
#include "moc_dlgprefwaveform.cpp"
#include "preferences/waveformsettings.h"
#include "util/db/dbconnectionpooled.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/waveformwidgetfactory.h"

namespace {
constexpr WaveformWidgetType::Type kDefaultWaveform = WaveformWidgetType::RGB;
} // anonymous namespace

DlgPrefWaveform::DlgPrefWaveform(
        QWidget* pParent,
        UserSettingsPointer pConfig,
        std::shared_ptr<Library> pLibrary)
        : DlgPreferencePage(pParent),
          m_pConfig(pConfig),
          m_pLibrary(pLibrary) {
    setupUi(this);

    // Waveform overview init
    waveformOverviewComboBox->addItem(tr("Filtered")); // "0"
    waveformOverviewComboBox->addItem(tr("HSV"));      // "1"
    waveformOverviewComboBox->addItem(tr("RGB"));      // "2"

    // Populate waveform options.
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();
    // We assume that the original type list order remains constant.
    // We will use the type index later on to set waveform types and to
    // update the combobox.
    QVector<WaveformWidgetAbstractHandle> types = factory->getAvailableTypes();
    for (int i = 0; i < types.size(); ++i) {
        if (types[i].getType() == WaveformWidgetType::Empty) {
            continue;
        }
        waveformTypeComboBox->addItem(types[i].getDisplayName(), types[i].getType());
    }
    // Sort the combobox items alphabetically
    waveformTypeComboBox->model()->sort(0);

    // Populate zoom options.
    for (int i = static_cast<int>(WaveformWidgetRenderer::s_waveformMinZoom);
            i <= static_cast<int>(WaveformWidgetRenderer::s_waveformMaxZoom);
            i++) {
        defaultZoomComboBox->addItem(QString::number(100 / static_cast<double>(i), 'f', 1) + " %");
    }

    // Populate untilMark options
    untilMarkAlignComboBox->addItem(tr("Top"));
    untilMarkAlignComboBox->addItem(tr("Center"));
    untilMarkAlignComboBox->addItem(tr("Bottom"));

    // The GUI is not fully setup so connecting signals before calling
    // slotUpdate can generate rebootMixxxView calls.
    // TODO(XXX): Improve this awkwardness.
    slotUpdate();

    connect(frameRateSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &DlgPrefWaveform::slotSetFrameRate);
    connect(endOfTrackWarningTimeSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &DlgPrefWaveform::slotSetWaveformEndRender);
    connect(beatGridAlphaSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &DlgPrefWaveform::slotSetBeatGridAlpha);
    connect(frameRateSlider,
            &QSlider::valueChanged,
            frameRateSpinBox,
            &QSpinBox::setValue);
    connect(frameRateSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            frameRateSlider,
            &QSlider::setValue);
    connect(endOfTrackWarningTimeSlider,
            &QSlider::valueChanged,
            endOfTrackWarningTimeSpinBox,
            &QSpinBox::setValue);
    connect(endOfTrackWarningTimeSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            endOfTrackWarningTimeSlider,
            &QSlider::setValue);
    connect(beatGridAlphaSlider,
            &QSlider::valueChanged,
            beatGridAlphaSpinBox,
            &QSpinBox::setValue);
    connect(beatGridAlphaSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            beatGridAlphaSlider,
            &QSlider::setValue);

    connect(useWaveformCheckBox,
            &QCheckBox::clicked,
            this,
            &DlgPrefWaveform::slotSetWaveformEnabled);

    connect(waveformTypeComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DlgPrefWaveform::slotSetWaveformType);

    connect(useAccelerationCheckBox,
            &QCheckBox::clicked,
            this,
            &DlgPrefWaveform::slotSetWaveformAcceleration);
    connect(splitLeftRightCheckBox,
            &QCheckBox::clicked,
            this,
            &DlgPrefWaveform::slotSetWaveformOptionSplitStereoSignal);
    connect(highDetailCheckBox,
            &QCheckBox::clicked,
            this,
            &DlgPrefWaveform::slotSetWaveformOptionHighDetail);
    connect(defaultZoomComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DlgPrefWaveform::slotSetDefaultZoom);
    connect(synchronizeZoomCheckBox,
            &QCheckBox::clicked,
            this,
            &DlgPrefWaveform::slotSetZoomSynchronization);
    connect(allVisualGain,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &DlgPrefWaveform::slotSetVisualGainAll);
    connect(lowVisualGain,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &DlgPrefWaveform::slotSetVisualGainLow);
    connect(midVisualGain,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &DlgPrefWaveform::slotSetVisualGainMid);
    connect(highVisualGain,
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this,
            &DlgPrefWaveform::slotSetVisualGainHigh);
    connect(normalizeOverviewCheckBox,
            &QCheckBox::toggled,
            this,
            &DlgPrefWaveform::slotSetNormalizeOverview);
    connect(factory,
            &WaveformWidgetFactory::waveformMeasured,
            this,
            &DlgPrefWaveform::slotWaveformMeasured);
    connect(waveformOverviewComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DlgPrefWaveform::slotSetWaveformOverviewType);
    connect(clearCachedWaveforms,
            &QAbstractButton::clicked,
            this,
            &DlgPrefWaveform::slotClearCachedWaveforms);
    connect(playMarkerPositionSlider,
            &QSlider::valueChanged,
            this,
            &DlgPrefWaveform::slotSetPlayMarkerPosition);
    connect(untilMarkShowBeatsCheckBox,
            &QCheckBox::toggled,
            this,
            &DlgPrefWaveform::slotSetUntilMarkShowBeats);
    connect(untilMarkShowTimeCheckBox,
            &QCheckBox::toggled,
            this,
            &DlgPrefWaveform::slotSetUntilMarkShowTime);
    connect(untilMarkAlignComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DlgPrefWaveform::slotSetUntilMarkAlign);
    connect(untilMarkTextPointSizeSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &DlgPrefWaveform::slotSetUntilMarkTextPointSize);

    setScrollSafeGuardForAllInputWidgets(this);
}

DlgPrefWaveform::~DlgPrefWaveform() {
}

void DlgPrefWaveform::slotSetWaveformOptions(
        allshader::WaveformRendererSignalBase::Option option, bool enabled) {
    int currentOption = m_pConfig->getValue(
            ConfigKey("[Waveform]", "waveform_options"),
            allshader::WaveformRendererSignalBase::None);
    m_pConfig->setValue(ConfigKey("[Waveform]", "waveform_options"),
            enabled ? currentOption |
                            option
                    : currentOption ^
                            option);
    auto type = static_cast<WaveformWidgetType::Type>(
            waveformTypeComboBox->currentData().toInt());
    auto* factory = WaveformWidgetFactory::instance();
    factory->setWidgetTypeFromHandle(
            factory->findHandleIndexFromType(type), true);
}

void DlgPrefWaveform::slotUpdate() {
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();

    bool isAccelerationEnabled = false;
    if (factory->isOpenGlAvailable() || factory->isOpenGlesAvailable()) {
        openGlStatusData->setText(factory->getOpenGLVersion());
        useAccelerationCheckBox->setEnabled(true);
        isAccelerationEnabled = m_pConfig->getValue(
                                        ConfigKey("[Waveform]", "use_hardware_acceleration"),
                                        factory->preferredBackend()) !=
                WaveformWidgetBackend::None;
        useAccelerationCheckBox->setChecked(isAccelerationEnabled);
    } else {
        openGlStatusData->setText(tr("OpenGL not available") + ": " + factory->getOpenGLVersion());
        useAccelerationCheckBox->setEnabled(false);
        useAccelerationCheckBox->setChecked(false);
    }

#ifdef __APPLE__
    if (!CmdlineArgs::Instance().getDeveloper() &&
            m_pConfig->getValue(
                    ConfigKey("[Waveform]", "use_hardware_acceleration"),
                    factory->preferredBackend()) ==
                    factory->preferredBackend()) {
        useAccelerationCheckBox->hide();
        waveformSpacer->changeSize(0, 20);
    } else {
        useAccelerationCheckBox->show();
        waveformSpacer->changeSize(20, 20);
    }
#endif

    // The combobox holds a list of [handle name, handle index]
    int currentIndex = waveformTypeComboBox->findData(factory->getType());
    if (currentIndex != -1 && waveformTypeComboBox->currentIndex() != currentIndex) {
        waveformTypeComboBox->setCurrentIndex(currentIndex);
    }

    bool useWaveform = factory->getType() != WaveformWidgetType::Empty;
    useWaveformCheckBox->setChecked(useWaveform);

    int currentOptions = m_pConfig->getValue(
            ConfigKey("[Waveform]", "waveform_options"),
            allshader::WaveformRendererSignalBase::None);
    WaveformWidgetBackend backend = m_pConfig->getValue(
            ConfigKey("[Waveform]", "use_hardware_acceleration"),
            factory->preferredBackend());
    updateWaveformAcceleration(factory->getType(), backend);
    updateWaveformOption(useWaveform, backend, currentOptions);
    waveformTypeComboBox->setEnabled(useWaveform);
    updateEnableUntilMark();

    frameRateSpinBox->setValue(factory->getFrameRate());
    frameRateSlider->setValue(factory->getFrameRate());
    endOfTrackWarningTimeSpinBox->setValue(factory->getEndOfTrackWarningTime());
    endOfTrackWarningTimeSlider->setValue(factory->getEndOfTrackWarningTime());
    synchronizeZoomCheckBox->setChecked(factory->isZoomSync());
    allVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::All));
    lowVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::Low));
    midVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::Mid));
    highVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::High));
    normalizeOverviewCheckBox->setChecked(factory->isOverviewNormalized());
    // Round zoom to int to get a default zoom index.
    defaultZoomComboBox->setCurrentIndex(static_cast<int>(factory->getDefaultZoom()) - 1);
    playMarkerPositionSlider->setValue(static_cast<int>(factory->getPlayMarkerPosition() * 100));
    beatGridAlphaSpinBox->setValue(factory->getBeatGridAlpha());
    beatGridAlphaSlider->setValue(factory->getBeatGridAlpha());

    untilMarkShowBeatsCheckBox->setChecked(factory->getUntilMarkShowBeats());
    untilMarkShowTimeCheckBox->setChecked(factory->getUntilMarkShowTime());
    untilMarkAlignComboBox->setCurrentIndex(
            WaveformWidgetFactory::toUntilMarkAlignIndex(
                    factory->getUntilMarkAlign()));
    untilMarkTextPointSizeSpinBox->setValue(factory->getUntilMarkTextPointSize());

    // By default we set RGB woverview = "2"
    int overviewType = m_pConfig->getValue(
            ConfigKey("[Waveform]","WaveformOverviewType"), 2);
    if (overviewType != waveformOverviewComboBox->currentIndex()) {
        waveformOverviewComboBox->setCurrentIndex(overviewType);
    }

    WaveformSettings waveformSettings(m_pConfig);
    enableWaveformCaching->setChecked(waveformSettings.waveformCachingEnabled());
    enableWaveformGenerationWithAnalysis->setChecked(
        waveformSettings.waveformGenerationWithAnalysisEnabled());
    calculateCachedWaveformDiskUsage();
}

void DlgPrefWaveform::slotApply() {
    ConfigValue overviewtype = ConfigValue(waveformOverviewComboBox->currentIndex());
    if (overviewtype != m_pConfig->get(ConfigKey("[Waveform]", "WaveformOverviewType"))) {
        m_pConfig->set(ConfigKey("[Waveform]", "WaveformOverviewType"), overviewtype);
    }
    WaveformSettings waveformSettings(m_pConfig);
    waveformSettings.setWaveformCachingEnabled(enableWaveformCaching->isChecked());
    waveformSettings.setWaveformGenerationWithAnalysisEnabled(
        enableWaveformGenerationWithAnalysis->isChecked());
}

void DlgPrefWaveform::slotResetToDefaults() {
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();

    int defaultIndex = waveformTypeComboBox->findData(
            kDefaultWaveform);
    if (defaultIndex != -1 && waveformTypeComboBox->currentIndex() != defaultIndex) {
        waveformTypeComboBox->setCurrentIndex(defaultIndex);
    }
    auto defaultBackend = factory->preferredBackend();
    useWaveformCheckBox->setChecked(true);
    waveformTypeComboBox->setEnabled(true);
    updateWaveformAcceleration(kDefaultWaveform, defaultBackend);
    updateWaveformOption(true, defaultBackend, allshader::WaveformRendererSignalBase::None);

    // Restore waveform backend and option setting instantly
    m_pConfig->setValue(ConfigKey("[Waveform]", "waveform_options"),
            allshader::WaveformRendererSignalBase::None);
    m_pConfig->setValue(ConfigKey("[Waveform]", "use_hardware_acceleration"),
            defaultBackend);
    factory->setWidgetTypeFromHandle(factory->findHandleIndexFromType(kDefaultWaveform), true);

    allVisualGain->setValue(1.0);
    lowVisualGain->setValue(1.0);
    midVisualGain->setValue(1.0);
    highVisualGain->setValue(1.0);

    // Default zoom level is 3 in WaveformWidgetFactory.
    defaultZoomComboBox->setCurrentIndex(3 + 1);

    synchronizeZoomCheckBox->setChecked(true);

    // RGB overview.
    waveformOverviewComboBox->setCurrentIndex(2);

    // Don't normalize overview.
    normalizeOverviewCheckBox->setChecked(false);

    // 60FPS is the default
    frameRateSlider->setValue(60);
    endOfTrackWarningTimeSlider->setValue(30);

    // Waveform caching enabled.
    enableWaveformCaching->setChecked(true);
    enableWaveformGenerationWithAnalysis->setChecked(false);

    // Beat grid alpha default is 90
    beatGridAlphaSlider->setValue(90);
    beatGridAlphaSpinBox->setValue(90);

    // 50 (center) is default
    playMarkerPositionSlider->setValue(50);
}

void DlgPrefWaveform::slotSetFrameRate(int frameRate) {
    WaveformWidgetFactory::instance()->setFrameRate(frameRate);
}

void DlgPrefWaveform::slotSetWaveformEndRender(int endTime) {
    WaveformWidgetFactory::instance()->setEndOfTrackWarningTime(endTime);
}

void DlgPrefWaveform::slotSetWaveformType(int index) {
    // Ignore sets for -1 since this happens when we clear the combobox.
    if (index < 0) {
        return;
    }
    auto type = static_cast<WaveformWidgetType::Type>(
            waveformTypeComboBox->itemData(index).toInt());
    auto* factory = WaveformWidgetFactory::instance();
    factory->setWidgetTypeFromHandle(factory->findHandleIndexFromType(type));

    auto backend = m_pConfig->getValue(
            ConfigKey("[Waveform]", "use_hardware_acceleration"),
            factory->preferredBackend());
    useAccelerationCheckBox->setChecked(backend !=
            WaveformWidgetBackend::None);

    int currentOptions = m_pConfig->getValue(
            ConfigKey("[Waveform]", "waveform_options"),
            allshader::WaveformRendererSignalBase::None);
    updateWaveformAcceleration(type, backend);
    updateWaveformOption(true, backend, currentOptions);
    updateEnableUntilMark();
}

void DlgPrefWaveform::slotSetWaveformEnabled(bool checked) {
    auto* factory = WaveformWidgetFactory::instance();
    if (!checked) {
        factory->setWidgetTypeFromHandle(
                factory->findHandleIndexFromType(WaveformWidgetType::Empty),
                true);
    } else {
        auto type = static_cast<WaveformWidgetType::Type>(
                waveformTypeComboBox->currentData().toInt());
        factory->setWidgetTypeFromHandle(factory->findHandleIndexFromType(type), true);
    }
    slotUpdate();
}

void DlgPrefWaveform::slotSetWaveformAcceleration(bool checked) {
    WaveformWidgetBackend backend = WaveformWidgetBackend::None;
    if (checked) {
        backend =
#ifdef MIXXX_USE_QOPENGL
                WaveformWidgetBackend::AllShader
#else
                WaveformWidgetBackend::GL
#endif
                ;
    }
    m_pConfig->setValue(
            ConfigKey("[Waveform]", "use_hardware_acceleration"),
            backend);
    auto type = static_cast<WaveformWidgetType::Type>(waveformTypeComboBox->currentData().toInt());
    auto* factory = WaveformWidgetFactory::instance();
    factory->setWidgetTypeFromHandle(factory->findHandleIndexFromType(type), true);
    int currentOptions = m_pConfig->getValue(
            ConfigKey("[Waveform]", "waveform_options"),
            allshader::WaveformRendererSignalBase::None);
    updateWaveformOption(true, backend, currentOptions);
    updateEnableUntilMark();
}

void DlgPrefWaveform::updateWaveformAcceleration(
        WaveformWidgetType::Type type, WaveformWidgetBackend backend) {
    auto* factory = WaveformWidgetFactory::instance();
    int handleIdx = factory->findHandleIndexFromType(type);

    bool supportAcceleration = false, supportSoftware = true;
    if (handleIdx != -1) {
        const auto& handle = factory->getAvailableTypes()[handleIdx];
        supportAcceleration = handle.supportAcceleration();
        supportSoftware = handle.supportSoftware();
    }
    useAccelerationCheckBox->blockSignals(true);

    if (type == WaveformWidgetType::Empty) {
        useAccelerationCheckBox->setChecked(false);
    } else if (supportSoftware ^ supportAcceleration) {
        useAccelerationCheckBox->setChecked(!supportSoftware || supportAcceleration);
    } else {
        useAccelerationCheckBox->setChecked(backend != WaveformWidgetBackend::None);
    }

    useAccelerationCheckBox->setEnabled(supportAcceleration &&
            supportSoftware && type != WaveformWidgetType::Empty);

    useAccelerationCheckBox->blockSignals(false);
}
void DlgPrefWaveform::updateWaveformOption(
        bool useWaveform, WaveformWidgetBackend backend, int currentOption) {
    splitLeftRightCheckBox->blockSignals(true);
    highDetailCheckBox->blockSignals(true);

#ifdef MIXXX_USE_QOPENGL
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();
    int supportedOption = allshader::WaveformRendererSignalBase::None;

    auto type = static_cast<WaveformWidgetType::Type>(waveformTypeComboBox->currentData().toInt());
    int handleIdx = factory->findHandleIndexFromType(type);

    if (handleIdx != -1) {
        supportedOption = factory->getAvailableTypes()[handleIdx].supportedOptions(backend);
    }

    splitLeftRightCheckBox->setEnabled(useWaveform &&
            supportedOption &
                    allshader::WaveformRendererSignalBase::SplitStereoSignal);
    highDetailCheckBox->setEnabled(useWaveform &&
            supportedOption &
                    allshader::WaveformRendererSignalBase::HighDetail);
    splitLeftRightCheckBox->setChecked(splitLeftRightCheckBox->isEnabled() &&
            currentOption &
                    allshader::WaveformRendererSignalBase::SplitStereoSignal);
    highDetailCheckBox->setChecked(highDetailCheckBox->isEnabled() &&
            currentOption & allshader::WaveformRendererSignalBase::HighDetail);
#else
    splitLeftRightCheckBox->setVisible(false);
    highDetailCheckBox->setVisible(false);
#endif

    splitLeftRightCheckBox->blockSignals(false);
    highDetailCheckBox->blockSignals(false);
}

void DlgPrefWaveform::updateEnableUntilMark() {
#ifndef MIXXX_USE_QOPENGL
    const bool enabled = false;
#else
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();
    const bool enabled =
            WaveformWidgetFactory::instance()->widgetTypeSupportsUntilMark() &&
            m_pConfig->getValue(
                    ConfigKey("[Waveform]", "use_hardware_acceleration"),
                    factory->preferredBackend()) !=
                    WaveformWidgetBackend::None;
#endif
    untilMarkShowBeatsCheckBox->setEnabled(enabled);
    untilMarkShowTimeCheckBox->setEnabled(enabled);
    untilMarkAlignLabel->setEnabled(enabled);
    untilMarkAlignComboBox->setEnabled(enabled);
    untilMarkTextPointSizeLabel->setEnabled(enabled);
    untilMarkTextPointSizeSpinBox->setEnabled(enabled);
    requiresGLSLLabel->setVisible(!enabled);
}

void DlgPrefWaveform::slotSetWaveformOverviewType(int index) {
    m_pConfig->set(ConfigKey("[Waveform]", "WaveformOverviewType"), ConfigValue(index));
    emit reloadUserInterface();
}

void DlgPrefWaveform::slotSetDefaultZoom(int index) {
    WaveformWidgetFactory::instance()->setDefaultZoom(index + 1);
}

void DlgPrefWaveform::slotSetZoomSynchronization(bool checked) {
    WaveformWidgetFactory::instance()->setZoomSync(checked);
}

void DlgPrefWaveform::slotSetVisualGainAll(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::All,gain);
}

void DlgPrefWaveform::slotSetVisualGainLow(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::Low,gain);
}

void DlgPrefWaveform::slotSetVisualGainMid(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::Mid,gain);
}

void DlgPrefWaveform::slotSetVisualGainHigh(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::High,gain);
}

void DlgPrefWaveform::slotSetNormalizeOverview(bool normalize) {
    WaveformWidgetFactory::instance()->setOverviewNormalized(normalize);
}

void DlgPrefWaveform::slotWaveformMeasured(float frameRate, int droppedFrames) {
    frameRateAverage->setText(
            QString::number((double)frameRate, 'f', 2) + " : " +
            tr("dropped frames") + " " + QString::number(droppedFrames));
}

void DlgPrefWaveform::slotClearCachedWaveforms() {
    AnalysisDao analysisDao(m_pConfig);
    QSqlDatabase dbConnection = mixxx::DbConnectionPooled(m_pLibrary->dbConnectionPool());
    analysisDao.deleteAnalysesByType(dbConnection, AnalysisDao::TYPE_WAVEFORM);
    analysisDao.deleteAnalysesByType(dbConnection, AnalysisDao::TYPE_WAVESUMMARY);
    calculateCachedWaveformDiskUsage();
}

void DlgPrefWaveform::slotSetBeatGridAlpha(int alpha) {
    m_pConfig->setValue(ConfigKey("[Waveform]", "beatGridAlpha"), alpha);
    WaveformWidgetFactory::instance()->setDisplayBeatGridAlpha(alpha);
}

void DlgPrefWaveform::slotSetPlayMarkerPosition(int position) {
    // QSlider works with integer values, so divide the percentage given by the
    // slider value by 100 to get a fraction of the waveform width.
    WaveformWidgetFactory::instance()->setPlayMarkerPosition(position / 100.0);
}

void DlgPrefWaveform::slotSetUntilMarkShowBeats(bool checked) {
    WaveformWidgetFactory::instance()->setUntilMarkShowBeats(checked);
}

void DlgPrefWaveform::slotSetUntilMarkShowTime(bool checked) {
    WaveformWidgetFactory::instance()->setUntilMarkShowTime(checked);
}

void DlgPrefWaveform::slotSetUntilMarkAlign(int index) {
    WaveformWidgetFactory::instance()->setUntilMarkAlign(
            WaveformWidgetFactory::toUntilMarkAlign(index));
}

void DlgPrefWaveform::slotSetUntilMarkTextPointSize(int value) {
    WaveformWidgetFactory::instance()->setUntilMarkTextPointSize(value);
}

void DlgPrefWaveform::calculateCachedWaveformDiskUsage() {
    AnalysisDao analysisDao(m_pConfig);
    QSqlDatabase dbConnection = mixxx::DbConnectionPooled(m_pLibrary->dbConnectionPool());
    size_t numBytes = analysisDao.getDiskUsageInBytes(dbConnection, AnalysisDao::TYPE_WAVEFORM) +
            analysisDao.getDiskUsageInBytes(dbConnection, AnalysisDao::TYPE_WAVESUMMARY);

    // Display total cached waveform size in mebibytes with 2 decimals.
    QString sizeMebibytes = QString::number(
            numBytes / (1024.0 * 1024.0), 'f', 2);

    waveformDiskUsage->setText(
            tr("Cached waveforms occupy %1 MiB on disk.").arg(sizeMebibytes));
}
