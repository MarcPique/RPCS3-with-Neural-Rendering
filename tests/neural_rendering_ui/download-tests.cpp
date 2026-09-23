#include "neural_rendering_tab.h"
#include "neural_rendering_config.h"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>
#include <stdexcept>

static void require(bool value, const char* message)
{
	if (!value) throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	const QDir root(neural_rendering::root_path());
	const QStringList files{"Setup-Neural.ps1", "fetch-neural-runtime.ps1", "neural-install.log", "neural-install.log.rpcs3-backup",
		"ReShade.ini", "ReShade.ini.rpcs3-backup", "ReShadePreset.ini", "dlss5-feed.cfg", "neural-rendering.json",
		"ReShade64.dll", "dlss5-feed.addon64", "renodx-dlss5.addon64", "nvngx_dlssnr.dll", "nvngx_dlss.dll", "rpcs3-settings-only.addon64",
		"neural-rendering/ReShade64.json", "neural-rendering/VkLayer_feed_vk.json", "neural-rendering/VkLayer_feed_vk.dll",
		"reshade-shaders/Shaders/ReShade.fxh", "reshade-shaders/Shaders/DLSS5_Feed.fx", "reshade-shaders/Shaders/lumenite_Kernel.fx"};
	for (const QString& file : files) if (QFileInfo::exists(root.filePath(file))) return 2;
	if (QFileInfo::exists(root.filePath("rpcs3.exe"))) return 2;
	const auto write = [&](const QString& name, const QString& content)
	{
		QFile file(root.filePath(name));
		require(file.open(QIODevice::WriteOnly), "Cannot create fixture");
		require(file.write(content.toUtf8()) >= 0, "Cannot write fixture");
	};
	int warnings = 0;
	QTimer dismiss;
	QObject::connect(&dismiss, &QTimer::timeout, [&]()
	{
		for (auto* widget : QApplication::topLevelWidgets())
			if (auto* box = qobject_cast<QMessageBox*>(widget)) { ++warnings; box->reject(); }
	});
	dismiss.start(20);
	int result = 0;
	try
	{
		{
			neural_rendering_tab tab;
			tab.findChild<QPushButton*>("neural_download_components")->click();
			require(warnings == 1, "Missing setup script did not report an error");
		}
		write("fetch-neural-runtime.ps1", "# Local mock: no network or binary execution.\n");
		write("Setup-Neural.ps1", "Write-Output 'Simulated network failure'; exit 7\n");
		{
			neural_rendering_tab tab;
			tab.findChild<QCheckBox*>("neural_rendering_enabled")->click();
			require(warnings == 2, "Installer failure was not reported");
			require(!tab.findChild<QCheckBox*>("neural_rendering_enabled")->isChecked(), "Failed activation stayed checked");
			require(!neural_rendering::enabled(), "Failed install persisted activation");
		}
		write("Setup-Neural.ps1", "Write-Output 'Incomplete download'; exit 0\n");
		{
			neural_rendering_tab tab;
			tab.findChild<QPushButton*>("neural_download_components")->click();
			require(warnings == 3, "Successful exit without runtime files was accepted");
		}
		write("Setup-Neural.ps1", "Start-Sleep -Seconds 30\n");
		{
			neural_rendering_tab tab;
			QTimer cancel;
			QObject::connect(&cancel, &QTimer::timeout, [&]()
			{
				if (auto* button = tab.findChild<QPushButton*>("neural_download_cancel")) button->click();
			});
			cancel.start(500);
			tab.findChild<QCheckBox*>("neural_rendering_enabled")->click();
			require(!tab.findChild<QCheckBox*>("neural_rendering_enabled")->isChecked(), "Canceled activation stayed checked");
			require(warnings == 3, "Cancellation displayed an error");
		}
		QString script = "Set-Location -LiteralPath $PSScriptRoot\n";
		for (const QString& file : files.mid(9))
		{
			if (!file.contains('/') && !file.endsWith(".dll") && !file.endsWith(".addon64")) continue;
			script += QString("[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName((Join-Path $PSScriptRoot '%1'))) | Out-Null\n[IO.File]::WriteAllText((Join-Path $PSScriptRoot '%1'), 'fixture')\n").arg(file);
		}
		script += R"PS(
$reshade = '{"layer":{"name":"VK_LAYER_RPCS3_reshade","library_path":"..\\ReShade64.dll","disable_environment":{"DISABLE_VK_LAYER_RPCS3_reshade":"1"}}}'
$feeder = '{"layer":{"name":"VK_LAYER_feed_vk","library_path":".\\VkLayer_feed_vk.dll","disable_environment":{"DISABLE_VK_LAYER_feed_vk":"1"}}}'
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'neural-rendering/ReShade64.json'), $reshade)
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'neural-rendering/VkLayer_feed_vk.json'), $feeder)
exit 0
)PS";
		write("Setup-Neural.ps1", script);
		write("ReShade.ini", "[Custom]\nKeep=original\n");
		{
			neural_rendering_tab tab;
			auto* editor = tab.findChild<QPlainTextEdit*>("reshade_config_editor");
			editor->appendPlainText("Pending=edited");
			tab.findChild<QCheckBox*>("neural_rendering_enabled")->click();
			require(warnings == 3, "Complete runtime install was rejected");
			require(tab.findChild<QCheckBox*>("neural_rendering_enabled")->isChecked(), "Successful download did not retain requested activation");
			require(editor->toPlainText().contains("Pending=edited"), "Pending edit lost during download");
			require(!neural_rendering::read_text(root.filePath("ReShade.ini")).contains("Pending=edited"), "Download saved pending edits");
			require(!neural_rendering::enabled(), "Download saved activation before Apply");
			require(tab.save(), "Cannot save after automatic download");
			require(neural_rendering::enabled(), "Save did not enable completed runtime");
			require(neural_rendering::read_text(root.filePath("dlss5-feed.cfg")).contains("vk_present_sync=1"), "Fresh install lacks Vulkan feeder defaults");
		}
		{
			neural_rendering_tab tab(nullptr, []() { return false; });
			require(!tab.findChild<QPushButton*>("neural_download_components")->isEnabled(), "Download available during emulation");
		}
		QTextStream(stdout) << "PASS: missing scripts, failed/incomplete downloads, cancel, automatic checkbox download, pending edits retained, Apply after install, emulation guard.\n";
	}
	catch (const std::exception& error) { QTextStream(stderr) << "FAIL: " << error.what() << '\n'; result = 1; }
	for (const QString& file : files) if (QFileInfo::exists(root.filePath(file)) && !QFile::remove(root.filePath(file))) result = 1;
	for (const QString& dir : {QString("neural-rendering"), QString("reshade-shaders/Shaders"), QString("reshade-shaders")}) root.rmdir(dir);
	return result;
}
