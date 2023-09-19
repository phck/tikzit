/*
    TikZiT - a GUI diagram editor for TikZ
    Copyright (C) 2018 Aleks Kissinger

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "tikzit.h"
#include "tikzassembler.h"
#include "tikzstyles.h"
#include "previewwindow.h"
#include "latexprocess.h"
#include "util.h"

#include <QFile>
#include <QFileDialog>
#include <QSettings>
#include <QDebug>
#include <QMessageBox>
#include <QRegularExpression>
#include <QVersionNumber>
#include <QNetworkAccessManager>
#include <QColorDialog>

// application-level instance of Tikzit
Tikzit *tikzit;

// font to use for node labels
QFont Tikzit::LABEL_FONT("Courrier", 9);

Tikzit::Tikzit() : _styleFile("[no styles]"), _activeWindow(nullptr)
{
}

void Tikzit::init()
{
    QSettings settings("tikzit", "tikzit");

	initColors();
    initTexConstants();

    _mainMenu = new MainMenu();
    QMainWindow *dummy = new QMainWindow();

    _toolPalette = new ToolPalette(dummy);
    _propertyPalette = new PropertyPalette(dummy);
    //_stylePalette = new StylePalette(dummy);
    _styles = new TikzStyles(this);

    _styleEditor = new StyleEditor();

    //_stylePalette->show();
    _windows << new MainWindow();
    _windows[0]->show();
    _windows[0]->restorePosition();

    _styleFile = "";
    _styleFilePath = "";
    QString styleFile = settings.value("previous-tikzstyles-file").toString();
    if (!styleFile.isEmpty()) loadStyles(styleFile);

    QVariant check = settings.value("check-for-updates");
    if (check.isNull()) {
        int resp = QMessageBox::question(nullptr,
          tr("Check for updates"),
          tr("Would you like TikZiT to check for updates automatically?"
             " (You can always change this later in the Help menu.)"),
          QMessageBox::Yes | QMessageBox::Default,
          QMessageBox::No,
          QMessageBox::NoButton);
        check.setValue(resp == QMessageBox::Yes);
    }

    setCheckForUpdates(check.toBool());

    if (check.toBool()) {
        checkForUpdates(false);
    }

    _preview = new PreviewWindow();
    _latex = nullptr;
}

//QMenuBar *Tikzit::mainMenu() const
//{
//    return _mainMenu;
//}

QColor Tikzit::colorByIndex(int i)
{
    return _cols[i];
}

QColor Tikzit::colorByName(QString name)
{
    for (int i = 0; i < _colNames.length(); ++i) {
        if (_colNames[i] == name) return _cols[i];
    }

    QRegularExpression re(
      "^rgb\\s*,\\s*255\\s*:\\s*"
      "red\\s*,\\s*([0-9]+)\\s*;\\s*"
      "green\\s*,\\s*([0-9]+)\\s*;\\s*"
      "blue\\s*,\\s*([0-9]+)\\s*$"
    );

    QRegularExpressionMatch m = re.match(name);

    if (m.hasMatch()) {
        return QColor(
                m.captured(1).toInt(),
                m.captured(2).toInt(),
                m.captured(3).toInt());
    }

    return QColor();
}

QString Tikzit::nameForColor(QColor col)
{
    for (int i = 0; i < _colNames.length(); ++i) {
        if (_cols[i] == col) return _colNames[i];
    }

    // if the color is not recognised, return it in tikz-readable RBG format
    return "rgb,255: red,"+ QString::number(col.red()) +
            "; green," + QString::number(col.green()) +
            "; blue," + QString::number(col.blue());
}

void Tikzit::newTikzStyles()
{
    QSettings settings("tikzit", "tikzit");
    QFileDialog dialog;
    dialog.setDefaultSuffix("tikzstyles");
    dialog.setWindowTitle(tr("Create TikZ Style File"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setLabelText(QFileDialog::Accept, "Create");
    dialog.setNameFilter(tr("TiKZ Style File (*.tikzstyles)"));
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory(settings.value("previous-file-path").toString());
    dialog.setOption(QFileDialog::DontUseNativeDialog);

    if (dialog.exec() && !dialog.selectedFiles().isEmpty()) {
        QString fileName = dialog.selectedFiles()[0];
        TikzStyles *st = new TikzStyles(this);

        if (st->saveStyles(fileName)) {
            QFileInfo fi(fileName);
            _styleFile = fi.fileName();
            _styleFilePath = fi.absoluteFilePath();
            settings.setValue("previous-tikzstyles-file", fileName);
            settings.setValue("previous-tikzstyles-path", fi.absolutePath());
            _styles->deleteLater();
            _styles = st;

            foreach (MainWindow *w, _windows) {
                w->tikzScene()->reloadStyles();
            }
        } else {
            QMessageBox::warning(nullptr,
                "Could not write to style file.",
                "Could not write to: '" + fileName + "'. Check file permissions or choose a new location.");
        }
    }
}

ToolPalette *Tikzit::toolPalette() const
{
    return _toolPalette;
}

PropertyPalette *Tikzit::propertyPalette() const
{
    return _propertyPalette;
}

void Tikzit::newDoc()
{
    MainWindow *w = new MainWindow();
    w->show();
    w->restorePosition();
    _windows << w;
}

MainWindow *Tikzit::activeWindow() const
{
    return _activeWindow;
}

void Tikzit::setActiveWindow(MainWindow *activeWindow)
{
    _activeWindow = activeWindow;
}

void Tikzit::removeWindow(MainWindow *w)
{
    _windows.removeAll(w);
    if (_activeWindow == w) {
        if (_windows.isEmpty()) {
            _activeWindow = nullptr;
            // TODO: check if we should quit when last window closed
            quit();
        } else _activeWindow = _windows[0];
    }
}

bool Tikzit::dialogStatus() const {
    return _dialog_active;
}

void Tikzit::setDialogStatus(bool active) {
    _dialog_active = active;
}

void Tikzit::open()
{
    QSettings settings("tikzit", "tikzit");
    QString fileName = QFileDialog::getOpenFileName(nullptr,
                tr("Open File"),
                settings.value("previous-file-path").toString(),
                tr("TiKZ Files (*.tikz)"),
                nullptr,
                QFileDialog::DontUseNativeDialog);

	open(fileName);
}

void Tikzit::open(QString fileName)
{
	if (!fileName.isEmpty()) {
		if (_windows.size() == 1 &&
			_windows[0]->tikzDocument()->isClean() &&
            _windows[0]->tikzDocument()->shortName().isEmpty())
        {
			_windows[0]->open(fileName);
			_windows[0]->show();
        }
        else
        {
            bool found = false;
            foreach (MainWindow *w, _windows) {
                if (w->tikzDocument()->fileName() == fileName) {
                    w->raise();
                    w->activateWindow();
                    found = true;
                }
            }

            if (!found) {
                MainWindow *w = new MainWindow();
                _windows << w;
                w->show();
                w->restorePosition();
                w->open(fileName);
            }
		}
	}
}

void Tikzit::openTikzStyles() {
    QSettings settings("tikzit", "tikzit");
    QString fileName = QFileDialog::getOpenFileName(nullptr,
                tr("Open File"),
                settings.value("previous-tikzstyles-path").toString(),
                tr("TiKZ Style Files (*.tikzstyles)"),
                nullptr,
                QFileDialog::DontUseNativeDialog);

    if (!fileName.isEmpty()) {
        QFileInfo fi(fileName);
        if (fi.exists() && loadStyles(fileName)) {
            QSettings settings("tikzit", "tikzit");
            settings.setValue("previous-tikzstyles-path", fi.absolutePath());
            settings.setValue("previous-tikzstyles-file", fileName);
        }
    }
}

bool Tikzit::loadStyles(QString fileName)
{
    QFileInfo fi(fileName);
    if (fi.exists()) {
        TikzStyles *st = new TikzStyles(this);
        if (st->loadStyles(fileName)) {
            _styleFile = fi.fileName();
            _styleFilePath = fi.absoluteFilePath();
            _styles->deleteLater();
            _styles = st;

            foreach (MainWindow *w, _windows) {
                w->tikzScene()->reloadStyles();
            }
            return true;
        } else {
            QMessageBox::warning(nullptr,
                "Bad style file.",
                "Bad style file: '" + fileName + "'. Check the file is properly formatted and try to load it again.");
            return false;
        }

    } else {
        //settings.setValue("previous-tikzstyles-file", "");
        QMessageBox::warning(nullptr,
            "Style file not found.", "Could not open style file: '" + fileName + "'.");
        return false;
    }
}

void Tikzit::showStyleEditor()
{
    _styleEditor->open();
}

QString Tikzit::styleFile() const
{
    return _styleFile;
}

QString Tikzit::styleFilePath() const
{
    return _styleFilePath;
}

void Tikzit::updateRecentFiles()
{
    foreach (MainWindow *w, _windows) {
        w->menu()->updateRecentFiles();
    }
}

void Tikzit::clearRecentFiles()
{
    QSettings settings("tikzit", "tikzit");
    settings.setValue("recent-files", QStringList());
    updateRecentFiles();
}

void Tikzit::setCheckForUpdates(bool check)
{
    QSettings settings("tikzit", "tikzit");
    settings.setValue("check-for-updates", check);
    foreach (MainWindow *w, _windows) {
        w->menu()->updatesAction()->blockSignals(true);
        w->menu()->updatesAction()->setChecked(check);
        w->menu()->updatesAction()->blockSignals(false);
    }
}

void Tikzit::checkForUpdates(bool manual)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    if (manual) {
        connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(updateManual(QNetworkReply*)));
    } else {
        connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(updateAuto(QNetworkReply*)));
    }

    manager->get(QNetworkRequest(QUrl("https://tikzit.github.io/latest-version.txt")));
}

void Tikzit::updateAuto(QNetworkReply *reply)
{
    updateReply(reply, false);
}

void Tikzit::updateManual(QNetworkReply *reply)
{
    updateReply(reply, true);
}

void Tikzit::updateReply(QNetworkReply *reply, bool manual)
{
    if (!reply->isReadable()) return;

    QByteArray data = reply->read(200);
    QString strLatest = QString::fromUtf8(data).simplified();

    // check for valid version string and capture optional RC suffix
    QRegularExpression re("^[1-9]+(\\.[0-9]+)*(-[rR][cC]([0-9]+))?$");
    QRegularExpressionMatch m;
    m = re.match(TIKZIT_VERSION);

    // any non-RC versions are considered later than RC versions.
    int rcCurrent = (!m.captured(3).isEmpty()) ? m.captured(3).toInt() : 1000;

    m = re.match(strLatest);

    if (m.hasMatch()) {
        QVersionNumber current = QVersionNumber::fromString(TIKZIT_VERSION).normalized();
        QVersionNumber latest = QVersionNumber::fromString(strLatest).normalized();

        int rcLatest = (!m.captured(3).isEmpty()) ? m.captured(3).toInt() : 1000;

        if (latest > current || (latest == current && rcLatest > rcCurrent)) {
            // give the version string in standard format
            strLatest = QString::number(latest.majorVersion()) + "." +
                QString::number(latest.minorVersion()) + "." +
                QString::number(latest.microVersion());
            if (rcLatest != 1000) strLatest += "-rc" + QString::number(rcLatest);
            QMessageBox::information(nullptr,
              tr("Update available"),
              "<p><b>A new version of TikZiT is available!</b></p>"
              "<p><i>current version: " TIKZIT_VERSION "<br />"
              "latest version: " + strLatest + "</i></p>"
              "<p>Download it now from: "
              "<a href=\"https://tikzit.github.io\">tikzit.github.io</a>.</p>");
        }
    } else {
        // don't complain of invalid response for auto update check
        if (manual) {
            QMessageBox::warning(nullptr,
              tr("Invalid response"),
              "<p>Got invalid version response from "
              "<a href=\"https://tikzit.github.io\">tikzit.github.io</a>.</p>");
        }
    }
}

void Tikzit::makePreview()
{
    if (activeWindow()) {
        LatexProcess *oldProc = _latex;
        _latex = new LatexProcess(_preview, this);
        if (oldProc != nullptr) {
            oldProc->kill();
            oldProc->deleteLater();
        }

        connect(_latex, SIGNAL(previewFinished()), this, SLOT(cleanupLatex()));

        if (activeWindow()->tikzDocument()->isEmpty()) {
            _latex->makePreview("\\begin{tikzpicture}\n"
                                "  \\node [style=none] (0) at (0,0) {};\n"
                                "\\end{tikzpicture}\n");
        } else {
            _latex->makePreview(activeWindow()->tikzSource());
        }

        _preview->show();

        // restores position from settings the first time this is called, otherwise
        // do nothing.
        _preview->restorePosition();
        _preview->raise();
        _preview->activateWindow();
    }
}

void Tikzit::cleanupLatex()
{
    LatexProcess *oldProc = _latex;
    _latex = nullptr;
    if (oldProc != nullptr) {
        oldProc->deleteLater();
    }
}

void Tikzit::initColors()
{
    // 19 standard xcolor colours
    _colNames <<
		"black" <<
        "darkgray" <<
        "gray" <<
		"lightgray" <<
		"white" <<

		"red" <<
		"orange" <<
		"yellow" <<
        "green" <<
		"blue" <<
		"purple" <<

		"brown" <<
		"olive" <<
        "lime" <<
        "cyan" <<
        "teal" <<

		"magenta" <<
		"violet" <<
		"pink";

    _cols <<
        QColor::fromRgbF(0,0,0) <<
        QColor::fromRgbF(0.25,0.25,0.25) <<
        QColor::fromRgbF(0.5,0.5,0.5) <<
        QColor::fromRgbF(0.75,0.75,0.75) <<
        QColor::fromRgbF(1,1,1) <<

        QColor::fromRgbF(1,0,0) <<
        QColor::fromRgbF(1,0.5,0) <<
        QColor::fromRgbF(1,1,0) <<
        QColor::fromRgbF(0,1,0) <<
        QColor::fromRgbF(0,0,1) <<
        QColor::fromRgbF(0.75,0,0.25) <<

        QColor::fromRgbF(0.75,0.5,0.25) <<
        QColor::fromRgbF(0.5,0.5,0) <<
        QColor::fromRgbF(0.75,1,0) <<
        QColor::fromRgbF(0,1,1) <<
        QColor::fromRgbF(0,0.5,0.5) <<

        QColor::fromRgbF(1,0,1) <<
        QColor::fromRgbF(0.5,0,0.5) <<
        QColor::fromRgbF(1,0.75,0.75);

    for (int i = 0; i < 48; ++i) {
        QColorDialog::setStandardColor(i, QColor(Qt::white));
    }

    // grayscale in column 1
    int pos = 0;
    for (int i=0; i < 5; ++i) {
        QColorDialog::setStandardColor(pos, _cols[i]);
        pos += 1;
    }

    // rainbow in column 2
    pos = 6;
    for (int i=5; i < 11; ++i) {
        QColorDialog::setStandardColor(pos, _cols[i]);
        pos += 1;
    }

    // brown/green/teal spectrum in column 3
    pos = 12;
    for (int i=11; i < 16; ++i) {
        QColorDialog::setStandardColor(pos, _cols[i]);
        pos += 1;
    }

    // pinks in column 4
    pos = 18;
    for (int i=16; i < 19; ++i) {
        QColorDialog::setStandardColor(pos, _cols[i]);
        pos += 1;
    }
}

PreviewWindow *Tikzit::previewWindow() const
{
    return _preview;
}

//StylePalette *Tikzit::stylePalette() const
//{
//    return _stylePalette;
//}


TikzStyles *Tikzit::styles() const
{
    return _styles;
}

void Tikzit::quit()
{
    //_stylePalette->close();
    QApplication::quit();
}


