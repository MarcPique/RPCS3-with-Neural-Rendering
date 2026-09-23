#include "neural_rendering_tab.h"
#include "neural_rendering_config.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QSlider>
#include <QTabWidget>
#include <QTest>
#include <QTextStream>
#include <QTimer>

#include <functional>
#include <stdexcept>

namespace
{
	void require(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}

	void write(const QString& path, const QString& text)
	{
		QString error;
		require(neural_rendering::write_text(path, text, &error), qPrintable(error));
	}

	bool warning_from(const std::function<bool()>& action)
	{
		bool shown = false;
		QTimer dismiss;
		QObject::connect(&dismiss, &QTimer::timeout, [&shown]()
		{
			for (QWidget* widget : QApplication::topLevelWidgets())
			{
				if (auto* box = qobject_cast<QMessageBox*>(widget))
				{
					shown = true;
					box->accept();
				}
			}
		});
		dismiss.start(10);
		const bool result = action();
		require(shown, "Expected an error message");
		return result;
	}

	template <typename T>
	T* child(neural_rendering_tab& tab, const char* name)
	{
		T* result = tab.findChild<T*>(QString::fromLatin1(name));
		require(result != nullptr, name);
		return result;
	}
}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	// The offscreen Windows plugin does not enumerate installed fonts itself.
	const QDir fonts(QDir(qEnvironmentVariable("WINDIR", QStringLiteral("C:/Windows"))).filePath(QStringLiteral("Fonts")));
	for (const QString& name : {QStringLiteral("segoeui.ttf"), QStringLiteral("cour.ttf"), QStringLiteral("consola.ttf")})
	{
		if (QFileInfo::exists(fonts.filePath(name))) QFontDatabase::addApplicationFont(fonts.filePath(name));
	}
	app.setFont(QFont(QStringLiteral("Segoe UI"), 10));
	// The executable has its own sandbox output directory. Never replace existing user files.
	const QDir root(neural_rendering::root_path());
	const QStringList owned_files{
		QStringLiteral("ReShade.ini"), QStringLiteral("ReShadePreset.ini"),
		QStringLiteral("ReShade.ini.rpcs3-backup"), QStringLiteral("ReShadePreset.ini.rpcs3-backup"),
		QStringLiteral("dlss5-feed.cfg"), QStringLiteral("dlss5-feed.cfg.rpcs3-backup"),
		QStringLiteral("neural-rendering.json")};
	for (const QString& name : owned_files)
	{
		if (QFileInfo::exists(root.filePath(name)))
		{
			QTextStream(stderr) << "Refusing to modify an existing file: " << root.filePath(name) << '\n';
			return 2;
		}
	}
	if (QFileInfo::exists(root.filePath(QStringLiteral("rpcs3.exe"))) || QFileInfo::exists(root.filePath(QStringLiteral("ReShade64.dll"))))
	{
		QTextStream(stderr) << "Run the harness only in its dedicated build sandbox.\n";
		return 2;
	}

	int exit_code = 0;
	try
	{
		{
			neural_rendering_tab tab;
			require(child<QDoubleSpinBox>(tab, "reshade_NRIntensity")->value() == 1.0, "Absent intensity did not show its default");
			require(!child<QPlainTextEdit>(tab, "reshade_config_editor")->toPlainText().contains("NRIntensity="), "Opening sliders wrote default keys");
			child<QPushButton>(tab, "neural_apply_preset")->click();
			require(child<QCheckBox>(tab, "neural_rendering_enabled")->isChecked(), "Quick setup did not request portable activation");
			child<QPlainTextEdit>(tab, "reshade_config_editor")->setPlainText(QStringLiteral("[Unknown]\nkeep=edited\n"));
			child<QPlainTextEdit>(tab, "neural_feeder_editor")->setPlainText(QStringLiteral("mode=2\n"));
		}
		require(!QFileInfo::exists(neural_rendering::config_path()), "Cancel wrote ReShade.ini");
		require(!QFileInfo::exists(neural_rendering::preset_path()), "Cancel wrote ReShadePreset.ini");
		require(!QFileInfo::exists(root.filePath(QStringLiteral("dlss5-feed.cfg"))), "Cancel wrote dlss5-feed.cfg");
		require(!QFileInfo::exists(root.filePath(QStringLiteral("neural-rendering.json"))), "Cancel wrote enable state");

		const QString fixture = QStringLiteral("; preserve this comment\n[GENERAL]\nEffectSearchPaths=.\\custom\\**\nPerformanceMode=0\nPresetPath=.\\ReShadePreset.ini\n[UnknownAddon]\nFutureOption=42\n");
		write(neural_rendering::config_path(), fixture);
		write(neural_rendering::preset_path(), QStringLiteral("Techniques=Custom@Custom.fx\n[Custom.fx]\nUnrecognizedUniform=0.25\n"));
		write(root.filePath(QStringLiteral("dlss5-feed.cfg")), QStringLiteral("mode=1\nunknown_future_setting=7\n"));
		{
			neural_rendering_tab tab;
			auto* config = child<QPlainTextEdit>(tab, "reshade_config_editor");
			config->setPlainText(config->toPlainText() + "\n[GENERAL]\nPreprocessorDefinitions=MY_VECTOR=1,,2,DLSS5_MV_PROVIDER=0\n");
			auto* profiles = child<QComboBox>(tab, "neural_quick_preset");
			for (int i = 0; i < profiles->count(); ++i)
			{
				profiles->setCurrentIndex(i);
				child<QPushButton>(tab, "neural_apply_preset")->click();
				require(child<QCheckBox>(tab, "neural_rendering_enabled")->isChecked(), "Preset did not enable portable runtime");
				require(config->toPlainText().contains("FutureOption=42") && config->toPlainText().contains("MY_VECTOR=1,,2"), "Preset lost unrelated or escaped values");
				require(neural_rendering::ini_value(config->toPlainText(), "GENERAL", "PreprocessorDefinitions") == "DLSS5_MV_PROVIDER=3,MY_VECTOR=1,,2", "Preset failed to replace motion provider");
				require(neural_rendering::ini_value(config->toPlainText(), "GENERAL", "EffectSearchPaths").contains("custom"), "Preset lost custom shader search path");
				require(neural_rendering::ini_value(child<QPlainTextEdit>(tab, "reshade_preset_editor")->toPlainText(), {}, "Techniques") == "Lumenite_Kernel@lumenite_Kernel.fx,DLSS5_Feed@DLSS5_Feed.fx,Custom@Custom.fx", "Preset broke effect order or lost custom effects");
				require(child<QPlainTextEdit>(tab, "neural_feeder_editor")->toPlainText().contains("mode=2"), "Quick preset failed to configure Feeder");
			}
			require(child<QDoubleSpinBox>(tab, "reshade_NRLocalTone")->value() == 1.2 && child<QSlider>(tab, "reshade_NRLocalTone_slider")->value() == 120, "Preset did not synchronize numeric input and slider");
		}
		require(neural_rendering::read_text(neural_rendering::config_path()) == fixture, "Cancelling quick presets wrote config");
		require(!QFileInfo::exists(root.filePath("neural-rendering.json")), "Cancelling quick presets wrote activation");
		{
			neural_rendering_tab tab;
			tab.resize(920, 590);
			tab.show();
			app.processEvents();
			auto* paths = child<QLineEdit>(tab, "reshade_EffectSearchPaths");
			paths->setFocus();
			paths->selectAll();
			QTest::keyClicks(paths, ".\\new\\**");
			auto* config_editor = child<QPlainTextEdit>(tab, "reshade_config_editor");
			auto* intensity_slider = child<QSlider>(tab, "reshade_NRIntensity_slider");
			auto* intensity_number = child<QDoubleSpinBox>(tab, "reshade_NRIntensity");
			intensity_slider->setValue(125);
			require(intensity_number->value() == 1.25 && neural_rendering::ini_value(config_editor->toPlainText(), "RenoDX.DLSS5", "NRIntensity") == "1.25", "Slider did not synchronize INI and number");
			intensity_number->setValue(0.8);
			require(intensity_slider->value() == 80 && neural_rendering::ini_value(config_editor->toPlainText(), "RenoDX.DLSS5", "NRIntensity") == "0.80", "Numeric input did not synchronize slider and INI");
			QString raw = config_editor->toPlainText();
			neural_rendering::set_ini_value(raw, "RenoDX.DLSS5", "NRIntensity", "3.50");
			config_editor->setPlainText(raw);
			require(intensity_number->value() == 3.5 && intensity_slider->value() == 200 && config_editor->toPlainText() == raw, "Out-of-range INI was silently clamped");
			neural_rendering::set_ini_value(raw, "RenoDX.DLSS5", "NRIntensity", "invalid");
			config_editor->setPlainText(raw);
			require(config_editor->toPlainText() == raw, "Invalid raw INI was silently overwritten");
			neural_rendering::set_ini_value(raw, "RenoDX.DLSS5", "NRIntensity", "1.37");
			config_editor->setPlainText(raw);
			require(intensity_number->value() == 1.37 && intensity_slider->value() == 137, "Raw INI did not synchronize sliders");
			require(neural_rendering::ini_value(config_editor->toPlainText(), "GENERAL", "EffectSearchPaths") == ".\\new\\**", "Form edits did not update raw INI");
			config_editor->setPlainText(config_editor->toPlainText() + QStringLiteral("\n[INPUT]\nKeyOverlay=112,0,0,0\n"));
			require(!tab.findChild<QLineEdit*>("reshade_KeyOverlay"), "In-game menu shortcut must not be configurable");
			require(config_editor->isReadOnly(), "Config view should not require editing numbers");
			for (auto* number : tab.findChildren<QDoubleSpinBox*>())
			{
				require(number->isReadOnly(), "Numeric values must be displayed without manual typing");
				require(tab.findChild<QSlider*>(number->objectName() + "_slider"), "Numeric control is missing its slider");
			}
			auto* tabs = child<QTabWidget>(tab, "neural_rendering_editors");
			tabs->setCurrentIndex(1);
			auto* uplift = child<QCheckBox>(tab, "renodx_neural_uplift");
			uplift->setChecked(false);
			uplift->setChecked(true);
			require(neural_rendering::ini_value(config_editor->toPlainText(), "RenoDX.DLSS5", "NeuralUplift") == "1", "Neural toggle did not set the real add-on key");
			require(neural_rendering::ini_value(config_editor->toPlainText(), "RenoDX.DLSS5", "EnableHooks") == "2", "Neural activation did not set NGX hooks");
			require(!tab.findChild<QWidget*>(QStringLiteral("reshade_NRGlobalTone")), "4.7-only global tone exposed in 4.55 package");
			require(!tab.findChild<QWidget*>(QStringLiteral("reshade_NRDiffuseWhiteNits")), "4.7-only diffuse white exposed in 4.55 package");
			auto* mode = child<QComboBox>(tab, "feeder_mode");
			mode->setFocus();
			QTest::keyClick(mode, Qt::Key_Down);
			require(child<QPlainTextEdit>(tab, "neural_feeder_editor")->toPlainText().contains("mode=2"), "Feeder form did not update CFG");
			auto* delay = child<QSlider>(tab, "feeder_create_delay_slider");
			require(delay->value() == 60 && !child<QPlainTextEdit>(tab, "neural_feeder_editor")->toPlainText().contains("create_delay="), "Spin default wrote an absent key");
			delay->setValue(75);
			require(child<QPlainTextEdit>(tab, "neural_feeder_editor")->toPlainText().contains("create_delay=75"), "Spin edit did not update CFG");
			QString precise = config_editor->toPlainText();
			neural_rendering::set_ini_value(precise, "RenoDX.DLSS5", "NRPaperWhiteScale", "15.401");
			config_editor->setPlainText(precise);
			child<QSlider>(tab, "feeder_mv_scale_x_slider")->setValue(-150);
			child<QSlider>(tab, "feeder_gpu_timeout_ms_slider")->setValue(2500);
			require(neural_rendering::ini_value(child<QPlainTextEdit>(tab, "neural_feeder_editor")->toPlainText(), {}, "mv_scale_x") == "-1.50", "Signed movement slider wrote the wrong scale or section");
			require(neural_rendering::ini_value(child<QPlainTextEdit>(tab, "neural_feeder_editor")->toPlainText(), {}, "gpu_timeout_ms") == "2500", "Integer slider wrote a decimal timeout");
			require(tab.save(), "Save of valid edits failed");
			const QString saved = neural_rendering::read_text(neural_rendering::config_path());
			require(neural_rendering::ini_value(saved, "RenoDX.DLSS5", "NRPaperWhiteScale") == "15.401", "Untouched precise setting was rounded by the slider view");
			require(neural_rendering::ini_value(saved, "INPUT", "KeyOverlay") == "0,0,0,0", "Saving preserved an in-game menu shortcut");
			require(saved.contains("; preserve this comment") && saved.contains("FutureOption=42"), "Save dropped comments or unknown add-on keys");
			require(neural_rendering::read_text(neural_rendering::preset_path()).contains("UnrecognizedUniform=0.25"), "Save changed unknown preset keys");
			require(neural_rendering::read_text(root.filePath(QStringLiteral("dlss5-feed.cfg"))).contains("unknown_future_setting=7"), "Save lost unknown Feeder keys");
			require(tab.save(), "Second Apply failed");
			require(!QFileInfo::exists(root.filePath(QStringLiteral("neural-rendering.json"))), "Editing config changed enable state");
			if (app.arguments().contains(QStringLiteral("--screenshots")))
			{
				tabs->setCurrentIndex(0);
				app.processEvents();
				require(tab.grab().save(root.filePath(QStringLiteral("neural-rendering-ui.png"))), "Screenshot failed");
				tabs->setCurrentIndex(1);
				qobject_cast<QScrollArea*>(tabs->widget(1))->verticalScrollBar()->setValue(0);
				app.processEvents();
				require(tab.grab().save(root.filePath(QStringLiteral("neural-rendering-controls.png"))), "Neural screenshot failed");
				tabs->setCurrentWidget(config_editor);
				app.processEvents();
				require(tab.grab().save(root.filePath(QStringLiteral("neural-rendering-ini.png"))), "INI screenshot failed");
			}
		}

		{
			neural_rendering_tab tab;
			auto* editor = child<QPlainTextEdit>(tab, "reshade_config_editor");
			editor->setPlainText(editor->toPlainText() + QStringLiteral("\n[Local]\nchange=1\n"));
			const QString external = QStringLiteral("[External]\nchanged=1\n");
			write(neural_rendering::config_path(), external);
			require(!warning_from([&tab]() { return tab.save(); }), "Save overwrote an external edit");
			require(neural_rendering::read_text(neural_rendering::config_path()) == external, "External edit was lost");
		}

		{
			neural_rendering_tab tab(nullptr, []() { return false; });
			require(!child<QCheckBox>(tab, "neural_rendering_enabled")->isEnabled(), "Enable checkbox editable during emulation");
			require(!child<QPlainTextEdit>(tab, "reshade_config_editor")->isEnabled(), "INI editable during emulation");
			require(child<QPlainTextEdit>(tab, "neural_rendering_diagnostics")->isEnabled(), "Diagnostics inaccessible during emulation");
			require(tab.save(), "Unchanged settings blocked Apply during emulation");
		}
		{
			bool stopped = true;
			neural_rendering_tab tab(nullptr, [&stopped]() { return stopped; });
			child<QPlainTextEdit>(tab, "reshade_config_editor")->appendPlainText(QStringLiteral("; pending edit"));
			stopped = false;
			require(!warning_from([&tab]() { return tab.save(); }), "Saved pending edits after emulation started");
		}
		{
			neural_rendering_tab tab;
			child<QCheckBox>(tab, "neural_rendering_enabled")->setChecked(true);
			require(!warning_from([&tab]() { return tab.save(); }), "Enabled missing runtime");
			require(!neural_rendering::enabled(), "Missing runtime was marked enabled");
		}

		// Empty is a valid user file, distinct from missing.
		write(neural_rendering::config_path(), QString());
		{
			neural_rendering_tab tab;
			require(child<QPlainTextEdit>(tab, "reshade_config_editor")->toPlainText().isEmpty(), "Empty file replaced by defaults");
		}
		require(QFile::remove(neural_rendering::config_path()), "Cannot prepare read-error fixture");
		require(root.mkdir(QStringLiteral("ReShade.ini")), "Cannot create read-error fixture");
		write(root.filePath(QStringLiteral("neural-rendering.json")), QStringLiteral("{\"enabled\": true, \"schema\": 1}"));
		{
			neural_rendering_tab tab;
			require(!child<QPlainTextEdit>(tab, "reshade_config_editor")->isEnabled(), "Read failure left editor writable");
			require(tab.save(), "Unreadable but unchanged integration blocked other settings");
			auto* enabled = child<QCheckBox>(tab, "neural_rendering_enabled");
			require(enabled->isEnabled(), "Read error prevented disabling runtime");
			enabled->setChecked(false);
			require(tab.save() && !neural_rendering::enabled(), "Cannot disable runtime with unreadable config");
		}
		require(root.rmdir(QStringLiteral("ReShade.ini")), "Cannot clean read-error fixture");
		QTextStream(stdout) << "PASS: sliders/numeric input/raw INI synchronization, absent and out-of-range values, four quick presets, cancellation, escaped-list and unknown-key preservation, repeated Apply, external edits, running-state guards, runtime validation, empty files, read errors.\n";
	}
	catch (const std::exception& error)
	{
		QTextStream(stderr) << "FAIL: " << error.what() << '\n';
		exit_code = 1;
	}
	// Only exact files created by this harness are removed; never recurse.
	for (const QString& name : owned_files)
	{
		const QString path = root.filePath(name);
		if (QFileInfo(path).isDir()) root.rmdir(name);
		else if (QFileInfo::exists(path) && !QFile::remove(path)) exit_code = 1;
	}
	return exit_code;
}
