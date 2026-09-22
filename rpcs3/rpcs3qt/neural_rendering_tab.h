#pragma once

#include <QWidget>

#include <functional>
#include <vector>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTabWidget;

// All edits remain in memory until the settings dialog calls save().
class neural_rendering_tab : public QWidget
{
public:
	explicit neural_rendering_tab(QWidget* parent = nullptr, std::function<bool()> can_edit = {});
	bool save();

private:
	struct ini_field
	{
		QWidget* widget;
		QString section;
		QString key;
		QPlainTextEdit* editor;
		std::function<void(const QString&)> apply_value;
	};

	void refresh_fields();
	void refresh_status();
	void set_value(QPlainTextEdit* editor, const QString& section, const QString& key, const QString& value);
	bool show_error(const QString& message);

	std::function<bool()> m_can_edit;
	QCheckBox* m_enabled = nullptr;
	QCheckBox* m_performance_mode = nullptr;
	QCheckBox* m_neural_uplift = nullptr;
	QLabel* m_status = nullptr;
	QLabel* m_preset_notice = nullptr;
	QPlainTextEdit* m_config = nullptr;
	QPlainTextEdit* m_preset = nullptr;
	QPlainTextEdit* m_feeder = nullptr;
	QPlainTextEdit* m_diagnostics = nullptr;
	QTabWidget* m_editors = nullptr;
	std::vector<ini_field> m_fields;
	QString m_original_config;
	QString m_original_preset;
	QString m_original_feeder;
	QString m_load_error;
	bool m_original_enabled = false;
	bool m_config_existed = false;
	bool m_preset_existed = false;
	bool m_feeder_existed = false;
	bool m_refreshing = false;
};
