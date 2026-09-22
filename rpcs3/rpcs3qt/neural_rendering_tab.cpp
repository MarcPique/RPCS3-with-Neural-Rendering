#include "neural_rendering_tab.h"
#include "neural_rendering_config.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSlider>
#include <QTabWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#ifdef _WIN32
#include <qt_windows.h>
#endif

#include <utility>
#include <initializer_list>
#include <limits>
#include <algorithm>
#include <cmath>

namespace
{
	QString normalized_text(QString text)
	{
		return text.replace(QStringLiteral("\r\n"), QStringLiteral("\n")).replace(QChar('\r'), QChar('\n'));
	}

	QString feeder_path()
	{
		return QDir(neural_rendering::root_path()).filePath(QStringLiteral("dlss5-feed.cfg"));
	}
	QStringList reshade_list(const QString& text)
	{
		QStringList values;
		QString value;
		for (qsizetype i = 0; i < text.size(); ++i)
		{
			if (text[i] != ',') value += text[i];
			else if (i + 1 < text.size() && text[i + 1] == ',') { value += ','; ++i; }
			else { if (!value.isEmpty()) values.append(value); value.clear(); }
		}
		if (!value.isEmpty()) values.append(value);
		return values;
	}
	QString reshade_list_text(QStringList values)
	{
		for (QString& value : values) value.replace(",", ",,");
		return values.join(',');
	}
}

neural_rendering_tab::neural_rendering_tab(QWidget* parent, std::function<bool()> can_edit)
	: QWidget(parent)
	, m_can_edit(std::move(can_edit))
{
	setObjectName(QStringLiteral("neural_rendering_tab"));
	auto* layout = new QVBoxLayout(this);
	auto* introduction = new QLabel(tr("Integración experimental de ReShade para Vulkan. Los componentes de neural rendering de DLSS5oneclick requieren compatibilidad con Vulkan; cargar ReShade no confirma que DLSS-NR esté funcionando."), this);
	introduction->setWordWrap(true);
	layout->addWidget(introduction);

	m_enabled = new QCheckBox(tr("Cargar ReShade Vulkan en RPCS3 (requiere reiniciar RPCS3)"), this);
	m_enabled->setObjectName(QStringLiteral("neural_rendering_enabled"));
	m_original_enabled = neural_rendering::enabled();
	m_enabled->setChecked(m_original_enabled);
	layout->addWidget(m_enabled);

	m_status = new QLabel(this);
	m_status->setWordWrap(true);
	layout->addWidget(m_status);

	m_editors = new QTabWidget(this);
	m_editors->setObjectName(QStringLiteral("neural_rendering_editors"));
	layout->addWidget(m_editors, 1);
	auto* controls_scroll = new QScrollArea(m_editors);
	controls_scroll->setWidgetResizable(true);
	auto* controls = new QWidget(controls_scroll);
	auto* form = new QFormLayout(controls);
	form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	controls_scroll->setWidget(controls);
	m_editors->addTab(controls_scroll, tr("ReShade"));

	m_config = new QPlainTextEdit(this);
	m_config->setObjectName(QStringLiteral("reshade_config_editor"));
	m_preset = new QPlainTextEdit(this);
	m_preset->setObjectName(QStringLiteral("reshade_preset_editor"));
	m_feeder = new QPlainTextEdit(this);
	m_feeder->setObjectName(QStringLiteral("neural_feeder_editor"));
	m_feeder->setToolTip(tr("Formato del Feeder: clave=valor numérico, sin secciones, sin BOM y sin espacios antes del signo igual. Se conservan las claves desconocidas."));
	for (QPlainTextEdit* editor : {m_config, m_preset, m_feeder})
	{
		editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		editor->setLineWrapMode(QPlainTextEdit::NoWrap);
		editor->setMinimumHeight(200);
	}
	m_editors->addTab(m_config, QStringLiteral("ReShade.ini"));
	m_editors->addTab(m_preset, QStringLiteral("ReShadePreset.ini"));
	m_editors->addTab(m_feeder, QStringLiteral("dlss5-feed.cfg"));

	m_config_existed = QFileInfo::exists(neural_rendering::config_path());
	m_preset_existed = QFileInfo::exists(neural_rendering::preset_path());
	m_feeder_existed = QFileInfo::exists(feeder_path());
	QString error;
	m_original_config = m_config_existed ? neural_rendering::read_text(neural_rendering::config_path(), &error) : neural_rendering::default_config();
	if (!error.isEmpty()) m_load_error = error;
	error.clear();
	m_original_preset = m_preset_existed ? neural_rendering::read_text(neural_rendering::preset_path(), &error) : neural_rendering::default_preset();
	if (!error.isEmpty()) m_load_error += (m_load_error.isEmpty() ? QString() : QStringLiteral("\n")) + error;
	error.clear();
	m_original_feeder = m_feeder_existed ? neural_rendering::read_text(feeder_path(), &error) : QString();
	if (!error.isEmpty()) m_load_error += (m_load_error.isEmpty() ? QString() : QStringLiteral("\n")) + error;
	m_original_config = normalized_text(m_original_config);
	m_original_preset = normalized_text(m_original_preset);
	m_original_feeder = normalized_text(m_original_feeder);
	m_config->setPlainText(m_original_config);
	m_preset->setPlainText(m_original_preset);
	m_feeder->setPlainText(m_original_feeder);

	m_performance_mode = new QCheckBox(tr("Compilar efectos en modo rendimiento"), controls);
	m_performance_mode->setObjectName(QStringLiteral("reshade_PerformanceMode"));
	m_performance_mode->setToolTip(tr("GENERAL / PerformanceMode. Desactívalo para editar los parámetros de efectos en el overlay de ReShade."));
	form->addRow(m_performance_mode);
	connect(m_performance_mode, &QCheckBox::toggled, this, [this](bool checked)
	{
		if (!m_refreshing) set_value(m_config, QStringLiteral("GENERAL"), QStringLiteral("PerformanceMode"), checked ? QStringLiteral("1") : QStringLiteral("0"));
	});

	const auto add_field = [this](QFormLayout* target_form, const QString& title, const QString& section, const QString& key, QPlainTextEdit* editor, const QString& tooltip)
	{
		auto* field = new QLineEdit(target_form->parentWidget());
		field->setObjectName((editor == m_preset ? QStringLiteral("preset_") : editor == m_feeder ? QStringLiteral("feeder_") : QStringLiteral("reshade_")) + key);
		field->setToolTip(tooltip);
		target_form->addRow(title, field);
		m_fields.push_back({field, section, key, editor, [field](const QString& value) { field->setText(value); }});
		connect(field, &QLineEdit::textEdited, this, [this, section, key, editor](const QString& value)
		{
			set_value(editor, section, key, value);
		});
		return field;
	};
	const auto add_combo = [this](QFormLayout* target_form, const QString& title, const QString& section, const QString& key, QPlainTextEdit* editor, const std::initializer_list<std::pair<int, QString>>& choices, const QString& tooltip)
	{
		auto* field = new QComboBox(target_form->parentWidget());
		field->setObjectName((editor == m_feeder ? QStringLiteral("feeder_") : QStringLiteral("reshade_")) + key);
		field->setToolTip(tooltip);
		field->setPlaceholderText(tr("Predeterminado del add-on"));
		for (const auto& [value, label] : choices) field->addItem(label, value);
		const int known_count = field->count();
		target_form->addRow(title, field);
		m_fields.push_back({field, section, key, editor, [field, known_count](const QString& value)
		{
			while (field->count() > known_count) field->removeItem(field->count() - 1);
			if (value.isEmpty()) { field->setCurrentIndex(-1); return; }
			bool ok = false;
			const double numeric = value.section(QChar(','), 0, 0).toDouble(&ok);
			int index = -1;
			for (int i = 0; ok && i < known_count; ++i)
			{
				if (field->itemData(i).toDouble() == numeric) { index = i; break; }
			}
			if (index < 0)
			{
				field->addItem(tr("Valor del archivo: %1").arg(value), value);
				index = field->count() - 1;
			}
			field->setCurrentIndex(index);
		}});
		connect(field, &QComboBox::activated, this, [this, field, editor, section, key](int index)
		{
			if (index >= 0 && !m_refreshing) set_value(editor, section, key, field->itemData(index).toString());
		});
		return field;
	};
	const auto add_integer = [this](QFormLayout* target_form, const QString& title, const QString& key, QPlainTextEdit* editor, int fallback, const QString& tooltip)
	{
		auto* field = new QSpinBox(target_form->parentWidget());
		field->setObjectName(QStringLiteral("feeder_") + key);
		field->setRange(0, std::numeric_limits<int>::max());
		field->setKeyboardTracking(false);
		field->setToolTip(tooltip);
		target_form->addRow(title, field);
		m_fields.push_back({field, QString(), key, editor, [field, fallback](const QString& value)
		{
			bool ok = false;
			const int number = value.toInt(&ok);
			field->setValue(ok ? number : fallback);
		}});
		connect(field, &QSpinBox::valueChanged, this, [this, editor, key](int value)
		{
			if (!m_refreshing) set_value(editor, QString(), key, QString::number(value));
		});
		return field;
	};
	const auto add_slider = [this](QFormLayout* target_form, const QString& title, const QString& key,
		double minimum, double maximum, double fallback)
	{
		auto* row = new QWidget(target_form->parentWidget());
		auto* row_layout = new QHBoxLayout(row);
		row_layout->setContentsMargins(0, 0, 0, 0);
		auto* slider = new QSlider(Qt::Horizontal, row);
		slider->setObjectName(QStringLiteral("reshade_") + key + QStringLiteral("_slider"));
		slider->setAccessibleName(title);
		slider->setRange(qRound(minimum * 100), qRound(maximum * 100));
		slider->setPageStep(10);
		auto* number = new QDoubleSpinBox(row);
		number->setObjectName(QStringLiteral("reshade_") + key);
		number->setAccessibleName(title);
		number->setRange(minimum, maximum);
		number->setDecimals(2);
		number->setSingleStep(0.01);
		number->setKeyboardTracking(false);
		number->setMinimumWidth(85);
		auto* notice = new QLabel(row);
		notice->setObjectName(QStringLiteral("reshade_") + key + QStringLiteral("_notice"));
		row_layout->addWidget(new QLabel(QString::number(minimum, 'f', 2), row));
		row_layout->addWidget(slider, 1);
		row_layout->addWidget(new QLabel(QString::number(maximum, 'f', 2), row));
		row_layout->addWidget(number);
		row_layout->addWidget(notice);
		const QString help = tr("Desliza o escribe el valor; paso 0,01. Rango del panel Feeder: %1 a %2. Si la clave no existe, se muestra %3 sin escribirla. Los valores avanzados del INI se conservan hasta editar este control.")
			.arg(minimum, 0, 'f', 2).arg(maximum, 0, 'f', 2).arg(fallback, 0, 'f', 2);
		slider->setToolTip(help);
		number->setToolTip(help);
		target_form->addRow(title, row);
		m_fields.push_back({row, QStringLiteral("RenoDX.DLSS5"), key, m_config,
			[slider, number, notice, minimum, maximum, fallback](const QString& value)
			{
				const QSignalBlocker slider_blocker(slider), number_blocker(number);
				bool ok = false;
				const double parsed = value.section(QChar(','), 0, 0).toDouble(&ok);
				ok = ok && std::isfinite(parsed);
				const double shown = ok ? parsed : fallback;
				number->setRange(std::min(minimum, shown), std::max(maximum, shown));
				number->setValue(shown);
				slider->setValue(qRound(std::clamp(shown, minimum, maximum) * 100));
				notice->setText(value.isEmpty() ? QString() : !ok ? QObject::tr("INI no numérico") :
					(shown < minimum || shown > maximum) ? QObject::tr("Fuera de rango") : QString());
				notice->setToolTip(value);
				notice->setVisible(!notice->text().isEmpty());
			}});
		connect(slider, &QSlider::valueChanged, this, [this, number, notice, key, minimum, maximum](int value)
		{
			if (m_refreshing) return;
			const QSignalBlocker blocker(number);
			number->setRange(minimum, maximum);
			number->setValue(value / 100.0);
			notice->hide();
			set_value(m_config, QStringLiteral("RenoDX.DLSS5"), key, QString::number(value / 100.0, 'f', 2));
		});
		connect(number, &QDoubleSpinBox::valueChanged, this, [this, slider, notice, key, minimum, maximum](double value)
		{
			if (m_refreshing) return;
			const QSignalBlocker blocker(slider);
			slider->setValue(qRound(std::clamp(value, minimum, maximum) * 100));
			notice->setText(value < minimum || value > maximum ? tr("Fuera de rango") : QString());
			notice->setVisible(!notice->text().isEmpty());
			set_value(m_config, QStringLiteral("RenoDX.DLSS5"), key, QString::number(value, 'f', 2));
		});
	};
	const QString list_help = tr("Lista de ReShade separada por comas. Una coma literal se escribe como dos comas. Se conservan las claves desconocidas y los parámetros de los add-ons.");
	add_field(form, tr("Carpetas de efectos"), QStringLiteral("GENERAL"), QStringLiteral("EffectSearchPaths"), m_config, list_help);
	add_field(form, tr("Carpetas de texturas"), QStringLiteral("GENERAL"), QStringLiteral("TextureSearchPaths"), m_config, list_help);
	add_field(form, tr("Definiciones globales"), QStringLiteral("GENERAL"), QStringLiteral("PreprocessorDefinitions"), m_config, list_help);
	add_field(form, tr("Tecla del overlay"), QStringLiteral("INPUT"), QStringLiteral("KeyOverlay"), m_config, tr("Formato de ReShade: tecla virtual, Ctrl, Shift, Alt. Inicio/Home: 36,0,0,0."));
	add_field(form, tr("Tecla para alternar efectos"), QStringLiteral("INPUT"), QStringLiteral("KeyEffects"), m_config, tr("Formato de ReShade: tecla virtual, Ctrl, Shift, Alt. 0,0,0,0 desactiva el atajo."));
	add_field(form, tr("Carpeta de capturas"), QStringLiteral("SCREENSHOT"), QStringLiteral("SavePath"), m_config, tr("Ruta de las capturas de ReShade, relativa a la carpeta de RPCS3 o absoluta."));
	add_field(form, tr("Técnicas activas del preset"), QString(), QStringLiteral("Techniques"), m_preset, list_help + tr(" Cada técnica usa el formato Nombre@Archivo.fx."));
	add_field(form, tr("Orden de las técnicas"), QString(), QStringLiteral("TechniqueSorting"), m_preset, list_help);
	add_field(form, tr("Definiciones del preset"), QString(), QStringLiteral("PreprocessorDefinitions"), m_preset, list_help);

	m_preset_notice = new QLabel(controls);
	m_preset_notice->setWordWrap(true);
	form->addRow(m_preset_notice);
	auto* raw_help = new QLabel(tr("Los editores INI permiten cambiar todos los ajustes persistidos en estos archivos por ReShade y por los add-ons, incluidas sus secciones de neural rendering. La disponibilidad de cada opción depende del add-on instalado. Usa Aplicar o Guardar para guardar; cerrar esta ventana descarta los cambios pendientes."), controls);
	raw_help->setWordWrap(true);
	form->addRow(raw_help);

	auto* feeder_scroll = new QScrollArea(m_editors);
	feeder_scroll->setWidgetResizable(true);
	auto* feeder_controls = new QWidget(feeder_scroll);
	auto* feeder_form = new QFormLayout(feeder_controls);
	feeder_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	feeder_scroll->setWidget(feeder_controls);
	m_editors->insertTab(1, feeder_scroll, tr("Neural / Feeder"));
	struct quick_profile { QString name; double intensity; double tone; double structure; double skin; int style; };
	const std::vector<quick_profile> profiles{
		{tr("Suave"), 0.60, 0.90, 0.75, -1.00, 1},
		{tr("Equilibrado"), 1.00, 1.00, 1.00, -1.00, 0},
		{tr("Detalle"), 1.00, 1.05, 1.35, 0.25, 1},
		{tr("Cinematográfico"), 1.00, 1.20, 1.10, -1.00, 2},
	};
	auto* quick_row = new QWidget(feeder_controls);
	auto* quick_layout = new QHBoxLayout(quick_row);
	quick_layout->setContentsMargins(0, 0, 0, 0);
	auto* quick_choice = new QComboBox(quick_row);
	quick_choice->setObjectName(QStringLiteral("neural_quick_preset"));
	quick_choice->setAccessibleName(tr("Preset de configuración rápida"));
	for (const auto& profile : profiles) quick_choice->addItem(profile.name);
	quick_choice->setCurrentIndex(1);
	auto* quick_apply = new QPushButton(tr("Usar preset"), quick_row);
	quick_apply->setObjectName(QStringLiteral("neural_apply_preset"));
	quick_layout->addWidget(quick_choice, 1);
	quick_layout->addWidget(quick_apply);
	feeder_form->addRow(tr("Configuración rápida"), quick_row);
	auto* quick_preview = new QLabel(feeder_controls);
	quick_preview->setWordWrap(true);
	const auto preview_profile = [quick_preview, profiles](int index)
	{
		const auto& profile = profiles.at(index);
		quick_preview->setText(QObject::tr("Intensidad %1 · Tono %2 · Estructura %3 · Piel %4\nUsar preset prepara y activa ReShade y Feeder. Después pulsa Aplicar o Guardar y reinicia RPCS3 con Vulkan y la GPU NVIDIA. Son puntos de partida de este paquete, ajustables para cada juego.")
			.arg(profile.intensity, 0, 'f', 2).arg(profile.tone, 0, 'f', 2).arg(profile.structure, 0, 'f', 2).arg(profile.skin, 0, 'f', 2));
	};
	connect(quick_choice, &QComboBox::currentIndexChanged, this, preview_profile);
	preview_profile(quick_choice->currentIndex());
	feeder_form->addRow(quick_preview);
	connect(quick_apply, &QPushButton::clicked, this, [this, profiles, quick_choice]()
	{
		if (m_can_edit && !m_can_edit()) return;
		const auto& profile = profiles.at(quick_choice->currentIndex());
		QString config = m_config->toPlainText(), preset = m_preset->toPlainText(), feeder = m_feeder->toPlainText();
		const auto nr = [&config](const QString& key, const QString& value) { neural_rendering::set_ini_value(config, QStringLiteral("RenoDX.DLSS5"), key, value); };
		nr("NeuralUplift", "1"); nr("EnableHooks", "2"); nr("NREnableUpscaling", "0"); nr("NRPreset", "0");
		nr("NRStyle", QString::number(profile.style));
		nr("NRIntensity", QString::number(profile.intensity, 'f', 2));
		nr("NRLocalTone", QString::number(profile.tone, 'f', 2));
		nr("NRLocalStructure", QString::number(profile.structure, 'f', 2));
		nr("NRSkinStructure", QString::number(profile.skin, 'f', 2));
		nr("NRAutoMask", "1"); nr("NRUICorrection", "1");
		neural_rendering::set_ini_value(config, "ADDON", "AddonPath", ".\\");
		neural_rendering::set_ini_value(config, "GENERAL", "PresetPath", ".\\ReShadePreset.ini");
		for (const auto& key : {QStringLiteral("EffectSearchPaths"), QStringLiteral("TextureSearchPaths")})
		{
			QStringList paths = reshade_list(neural_rendering::ini_value(config, "GENERAL", key));
			const QString local_path = neural_rendering::ini_value(neural_rendering::default_config(), "GENERAL", key);
			if (!paths.contains(local_path)) paths.prepend(local_path);
			neural_rendering::set_ini_value(config, "GENERAL", key, reshade_list_text(paths));
		}
		// Preserve unrelated definitions and effects, with the motion provider before Feeder.
		for (auto [editor, section] : {std::pair{&config, QStringLiteral("GENERAL")}, std::pair{&preset, QString()}})
		{
			QStringList definitions = reshade_list(neural_rendering::ini_value(*editor, section, "PreprocessorDefinitions"));
			definitions.removeIf([](const QString& value) { return value.section('=', 0, 0).trimmed() == "DLSS5_MV_PROVIDER"; });
			definitions.prepend("DLSS5_MV_PROVIDER=3");
			neural_rendering::set_ini_value(*editor, section, "PreprocessorDefinitions", reshade_list_text(definitions));
		}
		for (const auto& key : {QStringLiteral("Techniques"), QStringLiteral("TechniqueSorting")})
		{
			QStringList techniques = reshade_list(neural_rendering::ini_value(preset, {}, key));
			techniques.removeAll("Lumenite_Kernel@lumenite_Kernel.fx");
			techniques.removeAll("DLSS5_Feed@DLSS5_Feed.fx");
			techniques.prepend("DLSS5_Feed@DLSS5_Feed.fx");
			techniques.prepend("Lumenite_Kernel@lumenite_Kernel.fx");
			neural_rendering::set_ini_value(preset, {}, key, reshade_list_text(techniques));
		}
		neural_rendering::set_ini_value(feeder, {}, "enabled", "1");
		neural_rendering::set_ini_value(feeder, {}, "mode", "2");
		neural_rendering::set_ini_value(feeder, {}, "vk_present_sync", "1");
		m_refreshing = true;
		m_config->setPlainText(config); m_preset->setPlainText(preset); m_feeder->setPlainText(feeder);
		m_enabled->setChecked(true);
		m_refreshing = false;
		refresh_fields();
	});
	m_neural_uplift = new QCheckBox(tr("Activar neural rendering de RenoDX (NeuralUplift)"), feeder_controls);
	m_neural_uplift->setObjectName(QStringLiteral("renodx_neural_uplift"));
	m_neural_uplift->setToolTip(tr("RenoDX.DLSS5 / NeuralUplift en ReShade.ini. Requiere cargar ReShade, componentes compatibles y reiniciar RPCS3. Esta opción solicita la evaluación; consulta los registros para confirmar su ejecución."));
	feeder_form->addRow(m_neural_uplift);
	connect(m_neural_uplift, &QCheckBox::toggled, this, [this](bool checked)
	{
		if (m_refreshing) return;
		set_value(m_config, QStringLiteral("RenoDX.DLSS5"), QStringLiteral("NeuralUplift"), checked ? QStringLiteral("1") : QStringLiteral("0"));
		if (checked)
		{
			set_value(m_config, QStringLiteral("RenoDX.DLSS5"), QStringLiteral("EnableHooks"), QStringLiteral("2"));
			set_value(m_config, QStringLiteral("RenoDX.DLSS5"), QStringLiteral("NREnableUpscaling"), QStringLiteral("0"));
			refresh_fields();
		}
	});
	add_slider(feeder_form, tr("Intensidad neural"), QStringLiteral("NRIntensity"), 0.0, 2.0, 1.0);
	add_slider(feeder_form, tr("Tono local"), QStringLiteral("NRLocalTone"), 0.0, 2.0, 1.0);
	add_slider(feeder_form, tr("Estructura local"), QStringLiteral("NRLocalStructure"), 0.0, 2.0, 1.0);
	add_slider(feeder_form, tr("Estructura de piel"), QStringLiteral("NRSkinStructure"), -1.0, 1.0, -1.0);
	add_field(feeder_form, tr("Hooks de RenoDX"), QStringLiteral("RenoDX.DLSS5"), QStringLiteral("EnableHooks"), m_config, tr("DLSS5oneclick usa EnableHooks=2 para el Feeder."));
	add_combo(feeder_form, tr("Reescalado de RenoDX"), QStringLiteral("RenoDX.DLSS5"), QStringLiteral("NREnableUpscaling"), m_config, {{0, tr("Desactivado")}, {1, tr("Activado")}}, tr("NREnableUpscaling controla el reescalado del add-on. La integración Vulkan usa 0 de forma predeterminada."));
	const auto nr_field = [this, &add_field, feeder_form](const QString& label, const QString& key, const QString& help)
	{
		return add_field(feeder_form, label, QStringLiteral("RenoDX.DLSS5"), key, m_config, help);
	};
	add_combo(feeder_form, tr("Preset neural"), QStringLiteral("RenoDX.DLSS5"), QStringLiteral("NRPreset"), m_config, {{0, tr("Predeterminado")}, {1, tr("Preset 1")}, {2, tr("Preset 2")}, {3, tr("Preset 3")}}, tr("Presets del modelo instalado."));
	add_combo(feeder_form, tr("Estilo neural"), QStringLiteral("RenoDX.DLSS5"), QStringLiteral("NRStyle"), m_config, {{0, tr("Predeterminado")}, {1, tr("Natural")}, {2, tr("Cinematográfico")}}, tr("El modo cinematográfico tiene un fallo conocido en algunas versiones 4.6 y posteriores."));
	add_combo(feeder_form, tr("Máscara automática"), QStringLiteral("RenoDX.DLSS5"), QStringLiteral("NRAutoMask"), m_config, {{0, tr("Desactivada")}, {1, tr("Activada")}}, tr("Control de máscara automática del modelo."));
	add_combo(feeder_form, tr("Corrección de interfaz"), QStringLiteral("RenoDX.DLSS5"), QStringLiteral("NRUICorrection"), m_config, {{0, tr("Desactivada")}, {1, tr("Activada")}}, tr("Control de protección/corrección de la interfaz del add-on."));
	nr_field(tr("Escala de blanco de referencia"), QStringLiteral("NRPaperWhiteScale"), tr("Rango orientativo 0 a 10; valor observado 1."));
	nr_field(tr("Intensidad de transferencia"), QStringLiteral("NRTransferStrength"), tr("Rango orientativo 0 a 1."));
	nr_field(tr("Intensidad de color"), QStringLiteral("NRColorStrength"), tr("Rango orientativo 0 a 1."));
	add_combo(feeder_form, tr("Modo de profundidad neural"), QStringLiteral("RenoDX.DLSS5"), QStringLiteral("NRDepthMode"), m_config, {{0, tr("Flag NGX")}, {1, tr("Normal")}, {2, tr("Invertida")}}, tr("Interpretación de profundidad del add-on."));
	nr_field(tr("Escala neural de movimiento X"), QStringLiteral("NRMVecScaleX"), tr("Rango orientativo 0 a 4; valor observado 1."));
	nr_field(tr("Escala neural de movimiento Y"), QStringLiteral("NRMVecScaleY"), tr("Rango orientativo 0 a 4; valor observado 1."));
	auto* feeder_help = new QLabel(tr("Los valores predeterminados no se escriben hasta que cambies un control. Los rangos de RenoDX son orientativos y dependen de su versión. RenoDX 4.55 no incluye los controles de tono global ni blanco difuso de 4.7. El editor conserva cualquier clave adicional. La reducción de resolución del Feeder es exclusiva de D3D11 y no se aplica a Vulkan."), feeder_controls);
	feeder_help->setWordWrap(true);
	feeder_form->addRow(feeder_help);
	const auto feeder_field = [this, &add_field, feeder_form](const QString& label, const QString& key, const QString& help)
	{
		return add_field(feeder_form, label, QString(), key, m_feeder, help);
	};
	add_combo(feeder_form, tr("Feeder"), QString(), QStringLiteral("enabled"), m_feeder, {{0, tr("Desactivado")}, {1, tr("Activado")}}, tr("Activación del transporte del Feeder."));
	add_combo(feeder_form, tr("Modo del Feeder"), QString(), QStringLiteral("mode"), m_feeder, {{0, tr("Inactivo")}, {1, tr("Solo transporte")}, {2, tr("DLSS completo")}}, tr("Modo de evaluación del Feeder."));
	add_combo(feeder_form, tr("HDR"), QString(), QStringLiteral("hdr"), m_feeder, {{-1, tr("Automático")}, {0, tr("SDR")}, {1, tr("HDR")}}, tr("Interpretación del espacio de color."));
	add_combo(feeder_form, tr("Profundidad invertida"), QString(), QStringLiteral("depth_inverted"), m_feeder, {{-1, tr("Automática")}, {0, tr("Normal")}, {1, tr("Invertida")}}, tr("Interpretación del búfer de profundidad."));
	add_combo(feeder_form, tr("Preset DLSS"), QString(), QStringLiteral("preset"), m_feeder, {{0, tr("Automático")}, {5, tr("E")}, {6, tr("F")}, {10, tr("J")}, {11, tr("K")}}, tr("Preset del modelo DLSS."));
	add_combo(feeder_form, tr("Sincronización Vulkan"), QString(), QStringLiteral("vk_present_sync"), m_feeder, {{0, tr("Desactivada")}, {1, tr("Activada")}}, tr("Sincronización de presentación de Vulkan."));
	add_integer(feeder_form, tr("Retardo de creación (fotogramas)"), QStringLiteral("create_delay"), m_feeder, 60, tr("Retardo inicial del Feeder; valor predeterminado 60 fotogramas."));
	add_integer(feeder_form, tr("Calentamiento (fotogramas)"), QStringLiteral("warmup_rebuild"), m_feeder, 180, tr("Fotogramas antes de reconstruir la evaluación; valor predeterminado 180."));
	add_integer(feeder_form, tr("Tiempo límite de GPU (ms)"), QStringLiteral("gpu_timeout_ms"), m_feeder, 2000, tr("Tiempo de espera de la GPU; valor predeterminado 2000 milisegundos."));
	feeder_field(tr("Escala de movimiento X"), QStringLiteral("mv_scale_x"), tr("Multiplicador horizontal de vectores de movimiento; 1.0 conserva la escala."));
	feeder_field(tr("Escala de movimiento Y"), QStringLiteral("mv_scale_y"), tr("Multiplicador vertical de vectores de movimiento; 1.0 conserva la escala."));

	connect(m_config, &QPlainTextEdit::textChanged, this, [this]() { if (!m_refreshing) refresh_fields(); });
	connect(m_preset, &QPlainTextEdit::textChanged, this, [this]() { if (!m_refreshing) refresh_fields(); });
	connect(m_feeder, &QPlainTextEdit::textChanged, this, [this]() { if (!m_refreshing) refresh_fields(); });
	refresh_fields();

	auto* diagnostics_page = new QWidget(this);
	auto* diagnostics_layout = new QVBoxLayout(diagnostics_page);
	m_diagnostics = new QPlainTextEdit(diagnostics_page);
	m_diagnostics->setObjectName(QStringLiteral("neural_rendering_diagnostics"));
	m_diagnostics->setReadOnly(true);
	diagnostics_layout->addWidget(m_diagnostics, 1);
	auto* refresh_button = new QPushButton(tr("Actualizar diagnóstico"), diagnostics_page);
	diagnostics_layout->addWidget(refresh_button);
	connect(refresh_button, &QPushButton::clicked, this, [this]() { refresh_status(); });
	m_editors->addTab(diagnostics_page, tr("Diagnóstico"));

	auto* footer = new QHBoxLayout;
	m_download = new QPushButton(tr("Descargar / reparar componentes"), this);
	m_download->setObjectName(QStringLiteral("neural_download_components"));
	m_download->setToolTip(tr("Descarga ReShade, Feeder, RenoDX, modelos y shaders desde sus autores. Conserva tus ajustes. Requiere conexión a Internet."));
	footer->addWidget(m_download);
	connect(m_download, &QPushButton::clicked, this, [this]() { install_components(); });
	connect(m_enabled, &QCheckBox::clicked, this, [this](bool checked)
	{
		if (checked && !neural_rendering::validate_runtime().isEmpty() && !install_components())
		{
			m_enabled->setChecked(false);
		}
	});
	auto* folder_button = new QPushButton(tr("Abrir carpeta de componentes"), this);
	footer->addWidget(folder_button);
	footer->addStretch();
	layout->addLayout(footer);
	connect(folder_button, &QPushButton::clicked, this, []()
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(neural_rendering::root_path()));
	});

	refresh_status();
	auto* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, [this]()
	{
		const bool may_toggle = (!m_can_edit || m_can_edit()) && (m_load_error.isEmpty() || m_original_enabled);
		if (m_enabled->isEnabled() != may_toggle) refresh_status();
	});
	timer->start(1000);
}

void neural_rendering_tab::set_value(QPlainTextEdit* editor, const QString& section, const QString& key, const QString& value)
{
	QString text = editor->toPlainText();
	neural_rendering::set_ini_value(text, section, key, value);
	m_refreshing = true;
	editor->setPlainText(text);
	m_refreshing = false;
}

void neural_rendering_tab::refresh_fields()
{
	m_refreshing = true;
	const QString config = m_config->toPlainText();
	// ReShade appends duplicate entries; scalar settings consume the first value.
	m_performance_mode->setChecked(neural_rendering::ini_value(config, QStringLiteral("GENERAL"), QStringLiteral("PerformanceMode")).section(QChar(','), 0, 0).toInt() != 0);
	m_neural_uplift->setChecked(neural_rendering::ini_value(config, QStringLiteral("RenoDX.DLSS5"), QStringLiteral("NeuralUplift"), QStringLiteral("1")).section(QChar(','), 0, 0).toInt() != 0);
	for (const ini_field& field : m_fields)
	{
		const QSignalBlocker blocker(field.widget);
		if (field.editor == m_feeder)
		{
			// The Feeder uses a flat, numeric CFG and later assignments override earlier ones.
			QString value;
			for (const QString& line : m_feeder->toPlainText().split(QChar('\n')))
			{
				if (line.startsWith(field.key + QChar('='))) value = line.mid(field.key.size() + 1).trimmed();
			}
			field.apply_value(value);
		}
		else field.apply_value(neural_rendering::ini_value(field.editor->toPlainText(), field.section, field.key));
	}
	const QString active_preset = neural_rendering::ini_value(config, QStringLiteral("GENERAL"), QStringLiteral("PresetPath"));
	const QString resolved_preset = QDir::cleanPath(QDir(neural_rendering::root_path()).absoluteFilePath(QString(active_preset).replace(QChar('\\'), QChar('/'))));
	if (!active_preset.isEmpty() && resolved_preset.compare(QDir::cleanPath(neural_rendering::preset_path()), Qt::CaseInsensitive) != 0)
	{
		m_preset_notice->setText(tr("ReShade tiene seleccionado otro preset: %1. Este panel edita ReShadePreset.ini; cambia GENERAL / PresetPath en el editor para usarlo.").arg(active_preset));
	}
	else
	{
		m_preset_notice->setText(tr("Preset que se edita: %1").arg(QDir::toNativeSeparators(neural_rendering::preset_path())));
	}
	m_refreshing = false;
}

void neural_rendering_tab::refresh_status()
{
	const bool stopped = !m_can_edit || m_can_edit();
	const bool editable = stopped && m_load_error.isEmpty();
	m_download->setEnabled(editable);
#ifndef _WIN32
	m_download->setEnabled(false);
#endif
	// Keep the recovery switch available if a damaged INI cannot be read.
	m_enabled->setEnabled(stopped && (m_load_error.isEmpty() || m_original_enabled));
	// Leave diagnostics accessible while a game is running.
	for (int i = 0; i < m_editors->count() - 1; i++) m_editors->widget(i)->setEnabled(editable);
	if (!m_load_error.isEmpty())
	{
		m_status->setText(tr("No se pudieron leer los ajustes. Cierra y vuelve a abrir Ajustes tras corregir el error:\n%1").arg(m_load_error));
	}
	else if (!stopped)
	{
		m_status->setText(tr("Detén la emulación para editar estos archivos. ReShade puede escribirlos mientras el juego está en ejecución."));
	}
	else
	{
		m_status->setText(tr("Los ajustes son globales para esta copia portable de RPCS3. Reinicia RPCS3 después de guardar para que el runtime vuelva a leerlos."));
	}
	m_diagnostics->setPlainText(neural_rendering::diagnostics());
}

bool neural_rendering_tab::show_error(const QString& message)
{
	QMessageBox::warning(this, tr("Neural rendering / ReShade"), message);
	return false;
}

bool neural_rendering_tab::install_components()
{
#ifndef _WIN32
	return show_error(tr("La descarga integrada requiere Windows x64."));
#else
	if (m_can_edit && !m_can_edit()) return show_error(tr("Detén la emulación antes de instalar componentes."));
	if (!m_load_error.isEmpty()) return show_error(m_load_error);
	// Do not replace libraries mapped into this process, even with emulation stopped.
	if (GetModuleHandleW(L"ReShade64.dll") || GetModuleHandleW(L"VkLayer_feed_vk.dll") ||
		GetModuleHandleW(L"renodx-dlss5.addon64") || GetModuleHandleW(L"dlss5-feed.addon64"))
	{
		return show_error(tr("Los componentes están en uso. Desactiva ReShade, guarda y reinicia RPCS3 antes de repararlos."));
	}
	const QDir root(neural_rendering::root_path());
	for (const QString& name : {QStringLiteral("Setup-Neural.ps1"), QStringLiteral("fetch-neural-runtime.ps1")})
	{
		if (!QFileInfo(root.filePath(name)).isFile())
			return show_error(tr("Falta %1 junto a rpcs3.exe. Extrae el ZIP completo del release, incluidos sus scripts y DLL.").arg(name));
	}

	QDialog dialog(this);
	dialog.setObjectName(QStringLiteral("neural_download_dialog"));
	dialog.setWindowTitle(tr("Descargar componentes Neural / ReShade"));
	dialog.resize(720, 420);
	auto* layout = new QVBoxLayout(&dialog);
	auto* label = new QLabel(tr("Descargando y verificando componentes desde sus autores. Tus ajustes se conservan."), &dialog);
	label->setWordWrap(true);
	layout->addWidget(label);
	auto* progress = new QProgressBar(&dialog);
	progress->setRange(0, 0);
	layout->addWidget(progress);
	auto* output = new QPlainTextEdit(&dialog);
	output->setObjectName(QStringLiteral("neural_download_output"));
	output->setReadOnly(true);
	output->setMaximumBlockCount(2000);
	layout->addWidget(output, 1);
	auto* cancel = new QPushButton(tr("Cancelar"), &dialog);
	cancel->setObjectName(QStringLiteral("neural_download_cancel"));
	layout->addWidget(cancel);
	QProcess process(&dialog);
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setWorkingDirectory(root.absolutePath());
	process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* arguments)
	{
		arguments->flags |= CREATE_NO_WINDOW;
	});
	bool succeeded = false;
	QString failure;
	const auto drain = [&]()
	{
		const QByteArray bytes = process.readAll();
		if (!bytes.isEmpty()) output->appendPlainText(QString::fromUtf8(bytes));
	};
	connect(&process, &QProcess::readyReadStandardOutput, &dialog, drain);
	connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
	connect(&process, &QProcess::errorOccurred, &dialog, [&](QProcess::ProcessError error)
	{
		failure = process.errorString();
		if (error == QProcess::FailedToStart) dialog.done(QDialog::Accepted);
	});
	connect(&process, &QProcess::finished, &dialog, [&](int code, QProcess::ExitStatus status)
	{
		drain();
		succeeded = status == QProcess::NormalExit && code == 0;
		if (!succeeded && failure.isEmpty()) failure = tr("El instalador terminó con código %1.").arg(code);
		dialog.done(QDialog::Accepted);
	});
	const QString powershell = QDir(qEnvironmentVariable("SystemRoot", QStringLiteral("C:/Windows")))
		.filePath(QStringLiteral("System32/WindowsPowerShell/v1.0/powershell.exe"));
	// Pass separate arguments, never concatenate user paths into a shell command.
	QTimer::singleShot(0, &dialog, [&]()
	{
		process.start(powershell, {QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
			QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-File"),
			QDir::toNativeSeparators(root.filePath(QStringLiteral("Setup-Neural.ps1"))),
			QStringLiteral("-RpcS3Directory"), QDir::toNativeSeparators(root.absolutePath()),
			QStringLiteral("-FromRpcS3Id"), QString::number(QCoreApplication::applicationPid())});
	});
	const bool canceled = dialog.exec() != QDialog::Accepted;
	if (process.state() != QProcess::NotRunning)
	{
		process.kill();
		process.waitForFinished(3000);
	}
	drain();
	neural_rendering::write_text(root.filePath(QStringLiteral("neural-install.log")), output->toPlainText());
	refresh_status();
	if (canceled)
	{
		m_status->setText(tr("Descarga cancelada. Puedes reintentar; no se ha guardado la activación."));
		return false;
	}
	if (!succeeded) return show_error(tr("No se pudieron instalar los componentes. Puedes reintentar.\n%1\n\n%2")
		.arg(failure, output->toPlainText().right(4000)));
	const QString runtime_error = neural_rendering::validate_runtime();
	if (!runtime_error.isEmpty()) return show_error(tr("La descarga terminó, pero faltan archivos válidos:\n%1").arg(runtime_error));
	// Host mode never writes INI/CFG or activation files. Pending edits stay intact.
	if (!m_feeder_existed && m_feeder->toPlainText().isEmpty())
		m_feeder->setPlainText(QStringLiteral("enabled=1\nmode=2\nvk_present_sync=1\n"));
	refresh_fields();
	m_status->setText(tr("Componentes instalados. Elige un preset si lo deseas, guarda y reinicia RPCS3 para activar la integración."));
	return true;
#endif
}

bool neural_rendering_tab::save()
{
	if (m_enabled->isChecked() && m_enabled->isChecked() != m_original_enabled &&
		!neural_rendering::validate_runtime().isEmpty() && (!m_can_edit || m_can_edit()))
	{
		QMessageBox prompt(QMessageBox::Information, tr("Neural rendering / ReShade"),
			tr("Faltan componentes o necesitan reparación. Descárgalos desde sus autores para activar la integración."), QMessageBox::Cancel, this);
		auto* download = prompt.addButton(tr("Descargar e instalar"), QMessageBox::AcceptRole);
		prompt.exec();
		if (prompt.clickedButton() != download || !install_components()) return false;
	}
	const QString config = m_config->toPlainText();
	const QString preset = m_preset->toPlainText();
	const QString feeder = m_feeder->toPlainText();
	const bool enabled = m_enabled->isChecked();
	const bool config_changed = config != m_original_config;
	const bool preset_changed = preset != m_original_preset;
	const bool feeder_changed = feeder != m_original_feeder;
	const bool enabled_changed = enabled != m_original_enabled;
	if (!config_changed && !preset_changed && !feeder_changed && !enabled_changed) return true;
	if (m_can_edit && !m_can_edit()) return show_error(tr("Detén la emulación antes de guardar los ajustes de ReShade."));
	if (enabled_changed && !enabled && !config_changed && !preset_changed && !feeder_changed)
	{
		// Disabling never needs to touch, read, or validate the runtime's INI files.
		QString error;
		if (!neural_rendering::save_enabled(false, &error)) return show_error(error);
		m_original_enabled = false;
		refresh_status();
		return true;
	}
	if (!m_load_error.isEmpty()) return show_error(m_load_error);

	// Refuse to overwrite files changed by an installer or a running ReShade instance.
	const auto unchanged = [this](const QString& path, bool existed, const QString& original)
	{
		if (QFileInfo::exists(path) != existed) return show_error(tr("El archivo cambió fuera de RPCS3: %1. Cierra y vuelve a abrir Ajustes para cargarlo.").arg(path));
		if (!existed) return true;
		QString error;
		const QString current = normalized_text(neural_rendering::read_text(path, &error));
		if (!error.isEmpty()) return show_error(error);
		if (current != original) return show_error(tr("El archivo cambió fuera de RPCS3: %1. Cierra y vuelve a abrir Ajustes para cargarlo.").arg(path));
		return true;
	};
	if (!unchanged(neural_rendering::config_path(), m_config_existed, m_original_config) ||
		!unchanged(neural_rendering::preset_path(), m_preset_existed, m_original_preset) ||
		!unchanged(feeder_path(), m_feeder_existed, m_original_feeder)) return false;
	if (enabled && enabled_changed)
	{
		const QString runtime_error = neural_rendering::validate_runtime();
		if (!runtime_error.isEmpty()) return show_error(runtime_error);
	}

	QString error;
	const bool write_config = config_changed || (enabled && !m_config_existed);
	const bool write_preset = preset_changed || (enabled && !m_preset_existed);
	bool config_saved = false;
	bool preset_saved = false;
	bool feeder_saved = false;
	// Individual writes are atomic. Restore preceding writes if a later write fails.
	const auto rollback = [this, &config_saved, &preset_saved, &feeder_saved](QString& reason)
	{
		const auto restore = [&reason](const QString& path, bool existed, const QString& original)
		{
			QString restore_error;
			if (existed)
			{
				if (!neural_rendering::write_text(path, original, &restore_error)) reason += QStringLiteral("\n") + restore_error;
			}
			else if (!QFile::remove(path)) reason += tr("\nNo se pudo retirar el archivo recién creado: %1").arg(path);
		};
		if (feeder_saved) restore(feeder_path(), m_feeder_existed, m_original_feeder);
		if (preset_saved) restore(neural_rendering::preset_path(), m_preset_existed, m_original_preset);
		if (config_saved) restore(neural_rendering::config_path(), m_config_existed, m_original_config);
	};
	if (write_config)
	{
		if (!neural_rendering::write_text(neural_rendering::config_path(), config, &error)) return show_error(error);
		config_saved = true;
	}
	if (write_preset)
	{
		if (!neural_rendering::write_text(neural_rendering::preset_path(), preset, &error))
		{
			rollback(error);
			return show_error(error);
		}
		preset_saved = true;
	}
	if (feeder_changed)
	{
		if (!neural_rendering::write_text(feeder_path(), feeder, &error))
		{
			rollback(error);
			return show_error(error);
		}
		feeder_saved = true;
	}
	if (enabled_changed && !neural_rendering::save_enabled(enabled, &error))
	{
		rollback(error);
		return show_error(error);
	}
	m_original_config = config;
	m_original_preset = preset;
	m_original_feeder = feeder;
	m_original_enabled = enabled;
	m_config_existed = m_config_existed || write_config;
	m_preset_existed = m_preset_existed || write_preset;
	m_feeder_existed = m_feeder_existed || feeder_changed;
	refresh_status();
	return true;
}
