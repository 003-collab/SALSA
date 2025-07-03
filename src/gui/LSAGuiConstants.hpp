/*
	Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
	at Austin (ARL:UT).
	
	SALSA is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License version 3 (GPL-3.0-only) as published
	by the Free Software Foundation.
	
	SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along with
	SALSA.  If not, see https://www.gnu.org/licenses/.
*/
#ifndef LSAGUICONSTANTS_HPP
#define LSAGUICONSTANTS_HPP

#include <QString>
#include <QSettings>
#include <QColor>

#include "LSAConstants.hpp"

namespace lsa
{

const QString Q_CONTROLVALUE_NONE  = QString::fromStdString(CONTROLVALUE_NONE);
const QString Q_CONTROLVALUE_VALUE = QString::fromStdString(CONTROLVALUE_VALUE);
const QString Q_CONTROLVALUE_FROM  = QString::fromStdString(CONTROLVALUE_FROM);
const QString Q_CONTROLVALUE_AT    = QString::fromStdString(CONTROLVALUE_AT);
const QString Q_CONTROLVALUE_TO    = QString::fromStdString(CONTROLVALUE_TO);
const QString Q_CONTROLVALUE_LABEL = QString::fromStdString(CONTROLVALUE_LABEL);
const QString Q_CONTROLVALUE_MIXED = QString::fromStdString(CONTROLVALUE_MIXED);

const QString QSETTINGS_MAX_EXT_MAG_WARNING    = "QSETTINGS_MAX_EXT_MAG_WARNING";
const QString QSETTINGS_MAX_EXT_MAG_ERROR      = "QSETTINGS_MAX_EXT_MAG_ERROR";
const QString QSETTINGS_SHOW_MAP_CHECKED       = "QSETTINGS_SHOW_MAP_CHECKED";
const QString QSETTINGS_MAP_ENABLED            = "QSETTINGS_MAP_ENABLED";
const QString QSETTINGS_LASTPATH               = "QSETTINGS_LASTPATH";
const QString QSETTINGS_RECENTPROJECTS         = "QSETTINGS_RECENTPROJECTS";

// Font names used throughout SALSA
// TODO: Evaluate SALSA's general font paradigm #1193
const QString TREE_VIEW_FONT = "Monospace";
const QString SALSA_PRIMARY_FONT = "Cantarell";

//map options
// These variables hold the style data for all measurements and are edited by the user through OptionsDialog
//
// NOTE: Editing one of these variables through their public getter and setter methods will not automatically cause
// the map display to update.  Call updateStyles() to update the GeoDataStyles corresponding to these variables.
// Call update() to redraw the map with the new styles.
//
// NOTE: The pen styles are qint32 rather than Qt::PenStyle to facilitate consistent writing and reading of binary files.
// Cast these variables to Qt::PenStyle in order to use the enumeration.
const QString QSETTINGS_COLOR_AZIM          = "QSETTINGS_COLOR_AZIM";
const QString QSETTINGS_COLOR_AZIM_REF      = "QSETTINGS_COLOR_AZIM_REF";
const QString QSETTINGS_COLOR_HANG          = "QSETTINGS_COLOR_HANG";
const QString QSETTINGS_COLOR_HANG_REF      = "QSETTINGS_COLOR_HANG_REF";
const QString QSETTINGS_COLOR_ZANG          = "QSETTINGS_COLOR_ZANG";
const QString QSETTINGS_COLOR_DIST          = "QSETTINGS_COLOR_DIST";
const QString QSETTINGS_COLOR_DXYZ          = "QSETTINGS_COLOR_DXYZ";
const QString QSETTINGS_COLOR_HDIR          = "QSETTINGS_COLOR_HDIR";
const QString QSETTINGS_COLOR_HDIF          = "QSETTINGS_COLOR_HDIF";
const QString QSETTINGS_COLOR_VANG          = "QSETTINGS_COLOR_VANG";
const QString QSETTINGS_COLOR_ELLIPSE       = "QSETTINGS_COLOR_ELLIPSE";
const QString QSETTINGS_COLOR_RECTANGLE     = "QSETTINGS_COLOR_RECTANGLE";
const QString QSETTINGS_COLOR_HIGHLIGHT     = "QSETTINGS_COLOR_HIGHLIGHT";
const QString QSETTINGS_LINEWIDTH_AZIM      = "QSETTINGS_LINEWIDTH_AZIM";
const QString QSETTINGS_LINEWIDTH_AZIM_REF  = "QSETTINGS_LINEWIDTH_AZIM_REF";
const QString QSETTINGS_LINEWIDTH_HANG      = "QSETTINGS_LINEWIDTH_HANG";
const QString QSETTINGS_LINEWIDTH_HANG_REF  = "QSETTINGS_LINEWIDTH_HANG_REF";
const QString QSETTINGS_LINEWIDTH_ZANG      = "QSETTINGS_LINEWIDTH_ZANG";
const QString QSETTINGS_LINEWIDTH_DIST      = "QSETTINGS_LINEWIDTH_DIST";
const QString QSETTINGS_LINEWIDTH_DXYZ      = "QSETTINGS_LINEWIDTH_DXYZ";
const QString QSETTINGS_LINEWIDTH_HDIR      = "QSETTINGS_LINEWIDTH_HDIR";
const QString QSETTINGS_LINEWIDTH_HDIF      = "QSETTINGS_LINEWIDTH_HDIF";
const QString QSETTINGS_LINEWIDTH_VANG      = "QSETTINGS_LINEWIDTH_VANG";
const QString QSETTINGS_LINEWIDTH_ELLIPSE   = "QSETTINGS_LINEWIDTH_ELLIPSE";
const QString QSETTINGS_LINEWIDTH_RECTANGLE = "QSETTINGS_LINEWIDTH_RECTANGLE";
const QString QSETTINGS_PENSTYLE_AZIM       = "QSETTINGS_PENSTYLE_AZIM";
const QString QSETTINGS_PENSTYLE_AZIM_REF   = "QSETTINGS_PENSTYLE_AZIM_REF";
const QString QSETTINGS_PENSTYLE_HANG       = "QSETTINGS_PENSTYLE_HANG";
const QString QSETTINGS_PENSTYLE_HANG_REF   = "QSETTINGS_PENSTYLE_HANG_REF";
const QString QSETTINGS_PENSTYLE_ZANG       = "QSETTINGS_PENSTYLE_ZANG";
const QString QSETTINGS_PENSTYLE_DIST       = "QSETTINGS_PENSTYLE_DIST";
const QString QSETTINGS_PENSTYLE_DXYZ       = "QSETTINGS_PENSTYLE_DXYZ";
const QString QSETTINGS_PENSTYLE_HDIR       = "QSETTINGS_PENSTYLE_HDIR";
const QString QSETTINGS_PENSTYLE_HDIF       = "QSETTINGS_PENSTYLE_HDIF";
const QString QSETTINGS_PENSTYLE_VANG       = "QSETTINGS_PENSTYLE_VANG";
const QString QSETTINGS_PENSTYLE_ELLIPSE    = "QSETTINGS_PENSTYLE_ELLIPSE";
const QString QSETTINGS_PENSTYLE_RECTANGLE  = "QSETTINGS_PENSTYLE_RECTANGLE";
// colorHighlighting has no line width or pen style because all highlight placemarks use a
// line width derived from the line with of the placemark they are highlighting, and always
// use the Qt::SolidLine pen style (the default)
//map option defaults
const QColor DEFAULT_COLOR_AZIM           = QColor(84,84,84);
const QColor DEFAULT_COLOR_AZIM_REF       = QColor(84,84,84);
const QColor DEFAULT_COLOR_HANG           = QColor(84,84,84);
const QColor DEFAULT_COLOR_HANG_REF       = QColor(84,84,84);
const QColor DEFAULT_COLOR_ZANG           = QColor(213,94,0);
const QColor DEFAULT_COLOR_DIST           = QColor(213,94,0);
const QColor DEFAULT_COLOR_DXYZ           = QColor(0,114,178);
const QColor DEFAULT_COLOR_HDIR           = QColor(213,94,0);
const QColor DEFAULT_COLOR_HDIF           = QColor(213,94,0);
const QColor DEFAULT_COLOR_VANG           = QColor(213,94,0);
const QColor DEFAULT_COLOR_ELLIPSE        = QColor(255,0,0);
const QColor DEFAULT_COLOR_RECTANGLE      = QColor(85,255,255);
const QColor DEFAULT_COLOR_HIGHLIGHT      = QColor(243,224,2);
const qint32 DEFAULT_LINEWIDTH_AZIM       = 3;
const qint32 DEFAULT_LINEWIDTH_AZIM_REF   = 1;
const qint32 DEFAULT_LINEWIDTH_HANG       = 3;
const qint32 DEFAULT_LINEWIDTH_HANG_REF   = 1;
const qint32 DEFAULT_LINEWIDTH_ZANG       = 3;
const qint32 DEFAULT_LINEWIDTH_DIST       = 3;
const qint32 DEFAULT_LINEWIDTH_DXYZ       = 3;
const qint32 DEFAULT_LINEWIDTH_HDIR       = 3;
const qint32 DEFAULT_LINEWIDTH_HDIF       = 3;
const qint32 DEFAULT_LINEWIDTH_VANG       = 3;
const qint32 DEFAULT_LINEWIDTH_ELLIPSE    = 3;
const qint32 DEFAULT_LINEWIDTH_RECTANGLE  = 2;
const qint32 DEFAULT_PENSTYLE_AZIM        = Qt::SolidLine;
const qint32 DEFAULT_PENSTYLE_AZIM_REF    = Qt::SolidLine;
const qint32 DEFAULT_PENSTYLE_HANG        = Qt::SolidLine;
const qint32 DEFAULT_PENSTYLE_HANG_REF    = Qt::SolidLine;
const qint32 DEFAULT_PENSTYLE_ZANG        = Qt::DotLine;
const qint32 DEFAULT_PENSTYLE_DIST        = Qt::SolidLine;
const qint32 DEFAULT_PENSTYLE_DXYZ        = Qt::SolidLine;
const qint32 DEFAULT_PENSTYLE_HDIR        = Qt::SolidLine;
const qint32 DEFAULT_PENSTYLE_HDIF        = Qt::SolidLine;
const qint32 DEFAULT_PENSTYLE_VANG        = Qt::DotLine;
const qint32 DEFAULT_PENSTYLE_ELLIPSE     = Qt::SolidLine;
const qint32 DEFAULT_PENSTYLE_RECTANGLE   = Qt::SolidLine;


const int DEFAULT_CLI_TIMEOUT_SECONDS = 30;

const QString Q_UNITS_KM  = QString::fromStdString(UNITS_KM);
const QString Q_UNITS_M   = QString::fromStdString(UNITS_M);
const QString Q_UNITS_CM  = QString::fromStdString(UNITS_CM);
const QString Q_UNITS_FT  = QString::fromStdString(UNITS_FT);
const QString Q_UNITS_SOA = QString::fromStdString(UNITS_SOA);
const QString Q_UNITS_DEG = QString::fromStdString(UNITS_DEG);
const QString Q_UNITS_RAD = QString::fromStdString(UNITS_RAD);

const QString Q_OPTION_ELLIPSOID   = QString::fromStdString(OPTION_ELLIPSOID);
const QString Q_OPTION_ORTHOMETRIC = QString::fromStdString(OPTION_ORTHOMETRIC);

//preferences dialog settings
const QString QSETTINGS_ALLOW_AUTO_DISABLE_MAP = "QSETTINGS_ALLOW_AUTO_DISABLE_MAP";
const QString QSETTINGS_WARN_PREC_UPGRADE      = "QSETTINGS_WARN_PREC_UPGRADE";
const QString QSETTINGS_CLI_TIMEOUT_SECONDS    = "QSETTINGS_CLI_TIMEOUT_SECONDS";

//converter config custom values
const QString QSETTINGS_CUSTOM_GPSSTATIC_FROMC_M           = "QSETTINGS_CUSTOM_GPSSTATIC_FROMC_M";
const QString QSETTINGS_CUSTOM_GPSSTATIC_TOC_M             = "QSETTINGS_CUSTOM_GPSSTATIC_TOC_M";
const QString QSETTINGS_CUSTOM_GPSKIMEMATIC_FROMC_M        = "QSETTINGS_CUSTOM_GPSKIMEMATIC_FROMC_M";
const QString QSETTINGS_CUSTOM_GPSKIMEMATIC_TOC_M          = "QSETTINGS_CUSTOM_GPSKIMEMATIC_TOC_M";
const QString QSETTINGS_CUSTOM_LEVELS_SIGMA_M              = "QSETTINGS_CUSTOM_LEVELS_SIGMA_M";
const QString QSETTINGS_CUSTOM_LEVELS_FROMC_M              = "QSETTINGS_CUSTOM_LEVELS_FROMC_M";
const QString QSETTINGS_CUSTOM_LEVELS_TOC_M                = "QSETTINGS_CUSTOM_LEVELS_TOC_M";
const QString QSETTINGS_CUSTOM_TOTSTA_DIST_IR_SIGMA_M      = "QSETTINGS_CUSTOM_TOTSTA_DIST_IR_SIGMA_M";
const QString QSETTINGS_CUSTOM_TOTSTA_DIST_RED_SIGMA_M     = "QSETTINGS_CUSTOM_TOTSTA_DIST_RED_SIGMA_M";
const QString QSETTINGS_CUSTOM_TOTSTA_DIST_NOEDM_SIGMA_M   = "QSETTINGS_CUSTOM_TOTSTA_DIST_NOEDM_SIGMA_M";
const QString QSETTINGS_CUSTOM_TOTSTA_DIST_FROMC_M         = "QSETTINGS_CUSTOM_TOTSTA_DIST_FROMC_M";
const QString QSETTINGS_CUSTOM_TOTSTA_DIST_TOC_M           = "QSETTINGS_CUSTOM_TOTSTA_DIST_TOC_M";
const QString QSETTINGS_CUSTOM_TOTSTA_HORZ_SIGMA_SOA       = "QSETTINGS_CUSTOM_TOTSTA_HORZ_SIGMA_SOA";
const QString QSETTINGS_CUSTOM_TOTSTA_HORZ_FROMC_M         = "QSETTINGS_CUSTOM_TOTSTA_HORZ_FROMC_M";
const QString QSETTINGS_CUSTOM_TOTSTA_HORZ_TOC_M           = "QSETTINGS_CUSTOM_TOTSTA_HORZ_TOC_M";
const QString QSETTINGS_CUSTOM_TOTSTA_VERT_SIGMA_SOA       = "QSETTINGS_CUSTOM_TOTSTA_VERT_SIGMA_SOA";
const QString QSETTINGS_CUSTOM_TOTSTA_VERT_FROMC_M         = "QSETTINGS_CUSTOM_TOTSTA_VERT_FROMC_M";
const QString QSETTINGS_CUSTOM_TOTSTA_VERT_TOC_M           = "QSETTINGS_CUSTOM_TOTSTA_VERT_TOC_M";
const QString QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_SIGMA_M     = "QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_SIGMA_M";
const QString QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_FROMC_M     = "QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_FROMC_M";
const QString QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_TOC_M       = "QSETTINGS_CUSTOM_TOTSTA_ELEVDIFF_TOC_M";
const QString QSETTINGS_CUSTOM_TOTSTA_INSTR_HGHT_SIGMA_M   = "QSETTINGS_CUSTOM_TOTSTA_INSTR_HGHT_SIGMA_M";


//converter config saved settings
const QString QSETTINGS_COMBO_GPSSTATIC_FROMC               = "QSETTINGS_COMBO_GPSSTATIC_FROMC";
const QString QSETTINGS_COMBO_GPSKINEMATIC_FROMC            = "QSETTINGS_COMBO_GPSKINEMATIC_FROMC";
const QString QSETTINGS_COMBO_LEVELS_FROMC                  = "QSETTINGS_COMBO_LEVELS_FROMC";
const QString QSETTINGS_COMBO_GPSSTATIC_TOC                 = "QSETTINGS_COMBO_GPSSTATIC_TOC";
const QString QSETTINGS_COMBO_GPSKINEMATIC_TOC              = "QSETTINGS_COMBO_GPSKINEMATIC_TOC";
const QString QSETTINGS_COMBO_LEVELS_TOC                    = "QSETTINGS_COMBO_LEVELS_TOC";
const QString QSETTINGS_COMBO_TOTSTA_FROMC                  = "QSETTINGS_COMBO_TOTSTA_FROMC";
const QString QSETTINGS_COMBO_TOTSTA_TOC                    = "QSETTINGS_COMBO_TOTSTA_TOC";
const QString QSETTINGS_COMBO_LEVELS_MEASTYPE               = "QSETTINGS_COMBO_LEVELS_MEASTYPE";
const QString QSETTINGS_COMBO_LEVELS_SIGMASOURCE            = "QSETTINGS_COMBO_LEVELS_SIGMASOURCE";
const QString QSETTINGS_COMBO_TOTSTA_DIST_SIGMASOURCE       = "QSETTINGS_COMBO_TOTSTA_DIST_SIGMASOURCE";
const QString QSETTINGS_COMBO_TOTSTA_HORZ_SIGMASOURCE       = "QSETTINGS_COMBO_TOTSTA_HORZ_SIGMASOURCE";
const QString QSETTINGS_COMBO_TOTSTA_VERT_SIGMASOURCE       = "QSETTINGS_COMBO_TOTSTA_VERT_SIGMASOURCE";
const QString QSETTINGS_COMBO_TOTSTA_ELEVDIFF_SIGMASOURCE   = "QSETTINGS_COMBO_TOTSTA_ELEVDIFF_SIGMASOURCE";
const QString QSETTINGS_COMBO_TOTSTA_INSTR_HGHT_SIGMASOURCE = "QSETTINGS_COMBO_TOTSTA_ELEVDIFF_SIGMASOURCE";
const QString QSETTINGS_GPSSTATIC_PPM                       = "QSETTINGS_GPSSTATIC_PPM";
const QString QSETTINGS_LEVELS_PPM                          = "QSETTINGS_LEVELS_PPM";
const QString QSETTINGS_GPSKINEMATIC_PPM                    = "QSETTINGS_GPSKINEMATIC_PPM";
const QString QSETTINGS_DISTIR_PPM                          = "QSETTINGS_DISTIR_PPM";
const QString QSETTINGS_DISTRED_PPM                         = "QSETTINGS_DISTRED_PPM";
const QString QSETTINGS_DISTNON_PPM                         = "QSETTINGS_DISTNON_PPM";

//preferences panel

const int MAX_ERROR_STRING_SIZE = 300;
}
#endif // LSAGUICONSTANTS_HPP
