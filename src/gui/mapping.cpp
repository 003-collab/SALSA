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
#include "mapping.hpp"
#include "ui_mapping.h"
#include "optionsdialog.hpp"
#include "QThread" //debugging
#include <iostream>
using std::cout;
using std::endl;
#include <algorithm>

#include <marble/RemoteIconLoader.h>

//using namespace Marble;

const int Mapping::MEAS_LINE_WIDTH = 3;
const int Mapping::MEAS_LINE_WIDTH_HIGHLIGHTED = 7;
const int Mapping::MEAS_REF_LINE_WIDTH = 1;
const int Mapping::MEAS_REF_LINE_WIDTH_HIGHLIGHTED = 4;
const int Mapping::ELLIPSE_PRECISON = 32; // the number of line segments in an ellipse;
const int Mapping::ANGLE_PRECISION = 32; // the number of line segments in an 360 degree angle
const qreal Mapping::ANGLE_SCALE = 0.2; // the ratio of the radius of the angle symbol to the shortest distance in the angle
const qreal Mapping::ARROW_ANGLE = Pi / 6; // the angle in radians that the small parts of the arrow make with the main line
const qreal Mapping::ARROW_SCALE_FACTOR = 0.2; // how much smaller than the radius of the angle symbol the small parts of the arrow are drawn
const qreal Mapping::DEFAULT_HEIGHT = 0.0;  // the height points and measurements are at because marble does not display height information
const qreal Mapping::AZIM_LENGTH_FACTOR = 0.5; // the ratio of the length of the distance between the from and to points of an azimuth and how long to the north reference line is drawn
const qreal Mapping::MAX_FRACTION_OF_DIAGONAL = 0.5; // The maximum fraction of the mapping widget diagonal that the longest ellipse major axis and displacement vector can be
const qreal Mapping::DEFAULT_FRACTION_OF_DIAGONAL = 0.2; // The default fraction of the mapping widget diagonal that the longest ellipse major axis and displacement vector can be
const int Mapping::SCALE_BAR_WIDTH_PIXELS = 46; // the width in pixels of the black scale bar in the resource "scale_bar.png"

const double Mapping::ELLIPSE_MIN_RADIUS_THRESHOLD = 0.001;
const double Mapping::RECTANGLE_MIN_SIZE_THRESHOLD = 0.001;


const QString Mapping::PATH_FIXED_1 = ":/mapIcons/fixed_1.png";
const QString Mapping::PATH_FIXED_2 = ":/mapIcons/fixed_2.png";
const QString Mapping::PATH_FIXED_ALL = ":/mapIcons/fixed_all.png";
const QString Mapping::PATH_FIXEDC_1 = ":/mapIcons/fixedC_1.png";
const QString Mapping::PATH_FIXEDC_2 = ":/mapIcons/fixedC_2.png";
const QString Mapping::PATH_FIXEDAC = ":/mapIcons/fixedAC.png";
const QString Mapping::PATH_FLOATING = ":/mapIcons/floating.png";
const QString Mapping::PATH_DERIVED = ":/mapIcons/derived.png";
const QString Mapping::PATH_CONSTRAIN = ":/mapIcons/constrained.png";
const QString Mapping::PATH_APRIORI = ":/mapIcons/apriori.png";

const QString Mapping::PATH_FIXED_1_HIGH = ":/mapIcons/fixed_1_highlighted.png";
const QString Mapping::PATH_FIXED_2_HIGH = ":/mapIcons/fixed_2_highlighted.png";
const QString Mapping::PATH_FIXED_ALL_HIGH = ":/mapIcons/fixed_all_highlighted.png";
const QString Mapping::PATH_FIXEDC_1_HIGH = ":/mapIcons/fixedC_1_highlighted.png";
const QString Mapping::PATH_FIXEDC_2_HIGH = ":/mapIcons/fixedC_2_highlighted.png";
const QString Mapping::PATH_FIXEDAC_HIGH = ":/mapIcons/fixedAC_highlighted.png";
const QString Mapping::PATH_FLOATING_HIGH = ":/mapIcons/floating_highlighted.png";
const QString Mapping::PATH_DERIVED_HIGH = ":/mapIcons/derived_highlighted.png";
const QString Mapping::PATH_CONSTRAIN_HIGH = ":/mapIcons/constrained_highlighted.png";
const QString Mapping::PATH_APRIORI_HIGH = ":/mapIcons/apriori_highlighted.png";

const QString Mapping::MAP_THEME_BLANK = "earth/blank/blank.dgml";
const QString Mapping::MAP_THEME_OPENSTREET = "earth/openstreetmap/openstreetmap.dgml";

const int Mapping::HANDLE_ROW_REMOVED_TIMEOUT_MS  = 300;
const int Mapping::HANDLE_ROW_INSERTED_TIMEOUT_MS = 300;
const int Mapping::HANDLE_DATA_CHANGED_TIMEOUT_MS = 300;
const int Mapping::SET_HOME_TIMEOUT_MS = 300;

const int Mapping::SCROLL_ZOOM_THRESHOLD = 3460;
const int Mapping::NUM_SLIDER_DIVISIONS = 100;


/**
 * @brief The LSAVisualCategory enum
 * Marble made some big changes post version 16. One of these was to assign everything that gets drawn
 * its own Visual Category. These VCs are all predefined, and there's no way to add your own. If something
 * is not in a premade VC, it gets added to a list of leftovers to be drawn. There's no way to specify the
 * order in which those things get drawn in the list. These Highway VCs were chosen because there are
 * more than 20 of them which are meant to stay in the same order (and so drawn one after the other).
 * This way, we can ensure that different types of items on the map are consistently drawn one after
 * the other.
 */
enum LSAVisualCategory
{
    Default = GeoDataPlacemark::GeoDataVisualCategory::HighwaySteps,
    Highlight = GeoDataPlacemark::GeoDataVisualCategory::HighwayMotorwayLink,
    Selected = GeoDataPlacemark::GeoDataVisualCategory::HighwayMotorway
};

/**
 * @brief The LSAVisualCategoryOffset enum provides an offset from the LSAVisualCategory::Default, to ensure that
 * each type to be drawn on the map consistently has its own Visual Category.
 */
enum LSAVisualCategoryOffset
{
    // Lines
    DIST,
    DXYZ,
    ZANG,
    HDIF,
    HDIR,
    VANG,
    // Angles
    AZIM,
    HANG,
    // Other
    ELPS,
    RECT
};

/// Class Mapping encapsulates a Marble mapping widget with associated
/// standard behaviors.  In addition, Mapping impliments methods that
/// automate adding points and lines onto the map.
Mapping::Mapping(bool &saveLSARecord, QWidget *parent) : modelIsSaved(saveLSARecord) ,
    suppressMapWidgetUpdates(false), mapIsEnabled(true), currentlyShowingInitialCoordinates(true),
    QDockWidget(parent), ui(new Ui::Mapping),
    styleFixed1(new GeoDataStyle),
    styleFixed2(new GeoDataStyle),
    styleFixedAll(new GeoDataStyle),
    styleFloating(new GeoDataStyle),
    styleDerived(new GeoDataStyle),
    styleConstrain(new GeoDataStyle),
    styleAPriori(new GeoDataStyle),
    styleFixed1Highlighted(new GeoDataStyle),
    styleFixed2Highlighted(new GeoDataStyle),
    styleFixedAllHighlighted(new GeoDataStyle),
    styleFloatingHighlighted(new GeoDataStyle),
    styleDerivedHighlighted(new GeoDataStyle),
    styleConstrainHighlighted(new GeoDataStyle),
    styleAPrioriHighlighted(new GeoDataStyle),
    DISTStyle(new GeoDataStyle), DISTStyleHighlighted(new GeoDataStyle),
    DXYZStyle(new GeoDataStyle), DXYZStyleHighlighted(new GeoDataStyle),
    ZANGStyle(new GeoDataStyle), ZANGStyleHighlighted(new GeoDataStyle),
    HDIRStyle(new GeoDataStyle), HDIRStyleHighlighted(new GeoDataStyle),
    HDIFStyle(new GeoDataStyle), HDIFStyleHighlighted(new GeoDataStyle),
    VANGStyle(new GeoDataStyle), VANGStyleHighlighted(new GeoDataStyle),
    AZIMStyle(new GeoDataStyle), AZIMStyleHighlighted(new GeoDataStyle),
    AZIMReferenceLineStyle(new GeoDataStyle), AZIMReferenceLineStyleHighlighted(new GeoDataStyle),
    HANGStyle(new GeoDataStyle), HANGStyleHighlighted(new GeoDataStyle), HANGReferenceLineStyle(new GeoDataStyle),
    HANGReferenceLineStyleHighlighted(new GeoDataStyle),
    ellipseStyle(new GeoDataStyle),
    rectangleStyle(new GeoDataStyle),
    previousZoomLevel(0)
{

    // Find the marble data path
    QDir directory( qApp->applicationDirPath() );
    directory.cd("..");
    directory.cd("data/marble");
    MarbleDirs::setMarbleDataPath(directory.absolutePath() );

    QDir pluginDirectory( qApp->applicationDirPath() );
    pluginDirectory.cd("../plugins/marble");
    MarbleDirs::setMarblePluginPath(pluginDirectory.absolutePath() );


    // Qt Creator boilerplate
    ui->setupUi(this);

    // Set the map layer file location
    // In online mode, this will also automatically download the appropriate tiles (?)
    //ui->map->setMapThemeId("maps/earth/openstreetmap/openstreetmap.dgml");
    ui->map->setMapThemeId(MAP_THEME_BLANK);

    // imports a OSM data file
    // note: this command will automatically center the map on the osm data
    //ui->map->model()->addGeoDataFile("/home/whill/map2.osm");
    configureMap();

    // disables the default popup when a placemark is clicked
    ui->map->inputHandler()->setMouseButtonPopupEnabled(Qt::LeftButton,false);

    // disables the default context menu when the map is right clicked
    ui->map->inputHandler()->setMouseButtonPopupEnabled(Qt::RightButton,false);

    // disables map movement without direct mouse input
    ui->map->inputHandler()->setInertialEarthRotationEnabled(false);

    ui->map->setShowBackground(false); // removes the star layer


    // enables the download manager
    ui->map->model()->setWorkOffline(false);

    // connects the marble widget's left mouse button signal to a handler that decides what to do with it
    connect(ui->map, SIGNAL(mouseClickGeoPosition(qreal,qreal,GeoDataCoordinates::Unit)), this, SLOT(lmbHandler(qreal,qreal,GeoDataCoordinates::Unit)));

    // allows use of the customContextMenuRequested signal
    setContextMenuPolicy(Qt::CustomContextMenu);

    // connects the right mouse button to a method to show a context menu
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));

    // redraws ellipses and arrows whenever the zoom changes so that they are always drawn the same size on the screen
    connect(ui->map, SIGNAL(zoomChanged(int)), this, SLOT(handleZoomChange()));

    // connect the enable/disable button
    connect(ui->btnEnableDisable, SIGNAL(clicked()), this, SLOT(handleEnableDisableClicked()));

    // Connect lsatype checkbox controls
    connect(ui->checkDisplayPOSG, SIGNAL( clicked() ), this, SLOT( handlePOSG_Clicked() ) );
    connect(ui->checkDisplayPOSC, SIGNAL( clicked() ), this, SLOT( handlePOSC_Clicked() ) );
    connect(ui->checkDisplayDXYZ, SIGNAL( clicked() ), this, SLOT( handleDXYZ_Clicked() ) );
    connect(ui->checkDisplayDIST, SIGNAL( clicked() ), this, SLOT( handleDIST_Clicked() ) );
    connect(ui->checkDisplayHANG, SIGNAL( clicked() ), this, SLOT( handleHANG_Clicked() ) );
    connect(ui->checkDisplayHDIR, SIGNAL( clicked() ), this, SLOT( handleHDIR_Clicked() ) );
    connect(ui->checkDisplayZANG, SIGNAL( clicked() ), this, SLOT( handleZANG_Clicked() ) );
    connect(ui->checkDisplayAZIM, SIGNAL( clicked() ), this, SLOT( handleAZIM_Clicked() ) );
    connect(ui->checkDisplayHDIF, SIGNAL( clicked() ), this, SLOT( handleHDIF_Clicked() ) );
    connect(ui->checkDisplayVANG, SIGNAL( clicked() ), this, SLOT( handleVANG_Clicked() ) );
    connect(ui->checkDisplayRR, SIGNAL( clicked() ), this, SLOT( handleRR_Clicked() ) );
    connect(ui->checkDisplayEllipses, SIGNAL( clicked() ), this, SLOT( handleEllipses_Clicked() ) );

    connect(ui->radioDisplayInitial,  SIGNAL(clicked() ), this, SLOT (handleRadioInitialClicked() ) );
    connect(ui->radioDisplayAdjusted, SIGNAL(clicked() ), this, SLOT (handleRadioAdjustedClicked() ) );

    connect(ui->checkDisplayMapLayer, SIGNAL(clicked() ), this, SLOT (handleCheckDisplayMapLayerClicked()) );

    connect(ui->sliderEllipseScale, SIGNAL(sliderReleased()),     this, SLOT(handleSliderReleased()) );
    connect(ui->sliderEllipseScale, SIGNAL(actionTriggered(int)), this, SLOT(handleSliderMoved(int)) );

    // Adds the document to the map's treeModel for display
    ui->map->model()->treeModel()->addDocument( &mainDocument );

    // Make the title bar dark gray so the user can see it
    setStyleSheet("QDockWidget > QWidget { background: #e0e0e0;}"
                  "QDockWidget::title    { background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 lightgray, stop: 1 #e0e0e0);} ");

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    isPointMoving = false; // a point's position is not currently being changed
    movingPlacemark = NULL;

    configPath = QCoreApplication::applicationDirPath() + "/../data/config/";
    ellipseConfidenceFactor2D = 1.0;
    lastPointsSize = 0;

//    useDefaultStyle();

    iconStyleFixed1.setIconPath(PATH_FIXED_1);
    iconStyleFixed2.setIconPath(PATH_FIXED_2);
    iconStyleFixedAll.setIconPath(PATH_FIXED_ALL);
    iconStyleFloating.setIconPath(PATH_FLOATING);
    iconStyleDerived.setIconPath(PATH_DERIVED);
    iconStyleConstrain.setIconPath(PATH_CONSTRAIN);
    iconStyleAPriori.setIconPath(PATH_APRIORI);

    iconStyleFixed1Highlighted.setIconPath(PATH_FIXED_1_HIGH);
    iconStyleFixed2Highlighted.setIconPath(PATH_FIXED_2_HIGH);
    iconStyleFixedAllHighlighted.setIconPath(PATH_FIXED_ALL_HIGH);
    iconStyleFloatingHighlighted.setIconPath(PATH_FLOATING_HIGH);
    iconStyleDerivedHighlighted.setIconPath(PATH_DERIVED_HIGH);
    iconStyleConstrainHighlighted.setIconPath(PATH_CONSTRAIN_HIGH);
    iconStyleAPrioriHighlighted.setIconPath(PATH_APRIORI_HIGH);

    styleFixed1->setIconStyle(iconStyleFixed1);
    styleFixed2->setIconStyle(iconStyleFixed2);
    styleFixedAll->setIconStyle(iconStyleFixedAll);
    styleFloating->setIconStyle(iconStyleFloating);
    styleDerived->setIconStyle(iconStyleDerived);
    styleConstrain->setIconStyle(iconStyleConstrain);
    styleAPriori->setIconStyle(iconStyleAPriori);

    styleFixed1Highlighted->setIconStyle(iconStyleFixed1Highlighted);
    styleFixed2Highlighted->setIconStyle(iconStyleFixed2Highlighted);
    styleFixedAllHighlighted->setIconStyle(iconStyleFixedAllHighlighted);
    styleFloatingHighlighted->setIconStyle(iconStyleFloatingHighlighted);
    styleDerivedHighlighted->setIconStyle(iconStyleDerivedHighlighted);
    styleConstrainHighlighted->setIconStyle(iconStyleConstrainHighlighted);
    styleAPrioriHighlighted->setIconStyle(iconStyleAPrioriHighlighted);

    updateStyles();

    highScrollZoomEnabled = ui->map->zoom() >= SCROLL_ZOOM_THRESHOLD;

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    ui->checkDisplayMapLayer->setChecked(settings.value(lsa::QSETTINGS_SHOW_MAP_CHECKED, false).toBool());
    mapIsEnabled = settings.value(lsa::QSETTINGS_MAP_ENABLED, true).toBool();
    // Toggle the enabled state and button text
    if ( !mapIsEnabled )
    {
        ui->btnEnableDisable->setText("Enable");
    }

    // Enable/disable all of the map controls
    ui->map->setEnabled(mapIsEnabled);
    ui->frameCheckboxes->setEnabled(mapIsEnabled);
    ui->frameEllipseScaling->setEnabled(mapIsEnabled);
    ui->frameInputOutputRadios->setEnabled(mapIsEnabled);
    ui->checkDisplayMapLayer->setEnabled(mapIsEnabled);
    ui->checkDisplayRR->setEnabled(mapIsEnabled);

    if(ui->checkDisplayMapLayer->isChecked())
    {
        showBackgroundMap();
        adaptiveZoomThemeHandler();
    }


    suppressZoomUpdates = false;
    //    ui->map->setShowTileId(true); // uncomment to see all of the tile ids (useful for debugging)
    ui->map->setShowGrid(true);

    ui->map->setMapQualityForViewContext(Marble::NormalQuality, Marble::Still);
}

/// destructor to deallocate all memory from member pointer variables
Mapping::~Mapping()
{
    clear();

    delete ui;
}

void Mapping::setGuiModel(GuiModel *newModel)
{
    guiModel = newModel;
    connect(guiModel->selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(handleSelectionChanged(QItemSelection,QItemSelection) ));
    connect(guiModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),      this, SLOT(handleGuiModelDataChanged() ));
    connect(guiModel, SIGNAL(rowsInserted(QModelIndex, int, int)),       this, SLOT(handleGuiModelRowsInserted(QModelIndex, int, int) ));
    connect(guiModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(handleGuiModelRowsAboutToBeRemoved(QModelIndex,int,int) ));

    return;
}

void Mapping::initModel()
{
    if (!guiModel)
    {
        return;
    }

    initPlacemarks();

    updatePlacemarkVisibility();

    // Show the scale controls if we are diplaying output
    setScaleControlsVisible(ui->radioDisplayAdjusted->isChecked());

    setHome();
}

void Mapping::initPlacemarks(){
   // delete the contents of all internal containers
   clear();

   //recreate folders for mainDoc to use
   initFolders();

   // if we called update while a point is being moved, we are no longer moving that point
   isPointMoving = false;
   ui->map->setMouseTracking(true);
   movingPlacemark = NULL;
   ui->map->unsetCursor();

   copyOutputPositionsFromGuiModel();

   // Insert placemarks for all the points in the guiModel
   QModelIndex index;
   QList<QModelIndex> derivedPointIndexes;
   //insert points first then derived points, in case of duplication, to give points precedence in the map
   index = guiModel->getNextIndex();
   while ( index.isValid() )
   {
       LSAType lsaType = guiModel->getLSARecord(index)->getRecType();
       if ( lsaType == LSAType::POSC || lsaType == LSAType::POSG)
       {
           insertPlacemarkFromIndex(index);//first add points
       }
       else if (lsaType == LSAType::MEAN || lsaType == LSAType::ENUO)
       {
           derivedPointIndexes.append(index);
       }
       index = guiModel->getNextIndex(index);
   }
   foreach(QModelIndex indx, derivedPointIndexes)
   {
       insertPlacemarkFromIndex(indx);//now add derived points
   }

   // Insert placemarks for all the other records in the guiModel
   // don't show measurements on the adjusted view until an adjustment has been made - FIX TO BUG #990 HERE
   if(ui->radioDisplayInitial->isChecked() || (ui->radioDisplayAdjusted->isChecked() && (finalPointMap.size()>0)))
   {
       index = guiModel->getNextIndex();
       while ( index.isValid() )
       {
           LSAType lsaType = guiModel->getLSARecord(index)->getRecType();
           if ((lsaType != LSAType::POSC) && (lsaType != LSAType::POSG) && (lsaType != LSAType::MEAN) && (lsaType != LSAType::ENUO))
           {
               insertPlacemarkFromIndex(index);

               if (guiModel->getLSARecord(index)->getRecType() == LSAType::DGRP)
               {
                   QString label = QString::fromStdString(guiModel->getLSARecord(index)->getLabel());
                   dirGroupIndexMap.insert(label, QModelIndex(index));
               }
           }

           index = guiModel->getNextIndex(index);
       }
   }

   ui->map->model()->treeModel()->addDocument(&mainDocument);
}

void Mapping::initFolders(){
    //initilaze folder pointers, set name, and append to main doc
    //following new calls don't need matching delete calls, becuase the maindoc.clear() call inside mapping::clear() deletes any manual memory managed objects inside mainDocument
    pointsFolder = new GeoDataFolder();
    pointsFolder->setName(QString("POINTS"));
    mainDocument.append(pointsFolder);

    POSGFolder = new GeoDataFolder();
    POSGFolder->setName(QString("POSG"));
    pointsFolder->append(POSGFolder);

    POSCFolder = new GeoDataFolder();
    POSCFolder->setName(QString("POSC"));
    pointsFolder->append(POSCFolder);

    derivedPointsFolder = new GeoDataFolder();
    derivedPointsFolder->setName(QString("DERIVED POINTS"));
    mainDocument.append(derivedPointsFolder);

    meanFolder = new GeoDataFolder();
    meanFolder->setName(QString("MEAN"));
    derivedPointsFolder->append(meanFolder);

    enuoFolder = new GeoDataFolder();
    enuoFolder->setName(QString("ENUO"));
    derivedPointsFolder->append(enuoFolder);

    ellipseFolder = new GeoDataFolder();
    ellipseFolder->setName(QString("Ellipse"));
    pointsFolder->append(ellipseFolder);

    rectangleFolder = new GeoDataFolder();
    rectangleFolder->setName(QString("Rectangle"));
    pointsFolder->append(rectangleFolder);

    measurementFolder = new GeoDataFolder();
    measurementFolder->setName(QString("MEASUREMENTS"));
    mainDocument.append(measurementFolder);

    DISTFolder = new GeoDataFolder();
    DISTFolder->setName(QString("DIST"));
    measurementFolder->append(DISTFolder);

    DXYZFolder = new GeoDataFolder();
    DXYZFolder->setName(QString("DXYZ"));
    measurementFolder->append(DXYZFolder);

    HANGFolder = new GeoDataFolder();
    HANGFolder->setName(QString("HANG"));
    measurementFolder->append(HANGFolder);

    VANGFolder = new GeoDataFolder();
    VANGFolder->setName(QString("VANG"));
    measurementFolder->append(VANGFolder);

    ZANGFolder = new GeoDataFolder();
    ZANGFolder->setName(QString("ZANG"));
    measurementFolder->append(ZANGFolder);

    AZIMFolder = new GeoDataFolder();
    AZIMFolder->setName(QString("AZIM"));
    measurementFolder->append(AZIMFolder);

    HDIFFolder = new GeoDataFolder();
    HDIFFolder->setName(QString("HDIF"));
    measurementFolder->append(HDIFFolder);

    HDIRFolder = new GeoDataFolder();
    HDIRFolder->setName(QString("HDIR"));
    measurementFolder->append(HDIRFolder);

    //this folder will be used to remove all highlighted points before exporting the kml file
    //always put highlights folder in the last index of mainDocument
    highlightsFolder = new GeoDataFolder();
    highlightsFolder->setName(QString("ehighlights"));
    mainDocument.append(highlightsFolder);
}

void Mapping::resizeEvent(QResizeEvent *event)
{
    setHomeKeepCurrentView();

    QDockWidget::resizeEvent(event);
}

void Mapping::removePlacemark(QModelIndex index)
{

    // unhighlight all placemarks for this index
    unHighlightPlacemarksForIndex(index);

    // Identify the placemarks that need to be removed
    QList<GeoDataPlacemark*> placemarksToRemove = indexToPlacemarkMap.values(index);

    // If we are removing a point, remove the point's error ellipse
    LSARecord* lsaRecord = guiModel->getLSARecord(index);
    LSAType lsaType = lsaRecord->getRecType();
    bool isPointPlacemark =  (lsaType == LSAType::POSG || lsaType == LSAType::POSC || lsaType == LSAType::MEAN || lsaType == LSAType::ENUO);
    if (isPointPlacemark)
    {
        GeoDataPlacemark* ellipsePlacemark = ellipses.value(index);
        GeoDataPlacemark* ellipseVerticalBarsPlacemark = ellipseVerticalBars.value(index);
        placemarksToRemove.push_back(ellipsePlacemark);
        placemarksToRemove.push_back(ellipseVerticalBarsPlacemark);

        GeoDataPlacemark* rectanglePlacemark = rectangles.value(index);
        GeoDataPlacemark* rectangleVerticalBarsPlacemark = rectangleVerticalBars.value(index);
        placemarksToRemove.push_back(rectanglePlacemark);
        placemarksToRemove.push_back(rectangleVerticalBarsPlacemark);
    }

    // Remove the placemarks from the main document
    foreach(GeoDataPlacemark* placemark, placemarksToRemove)
    {
        QVector<GeoDataFolder*> folders = mainDocument.folderList();
        for (int folderIndex = 0; folderIndex < folders.size(); ++folderIndex)
        {
            GeoDataFolder* folder = folders.at(folderIndex);

            // Add any subfolders to our list
            folders.append(folder->folderList());

            QVector<GeoDataPlacemark*> placemarkList = folder->placemarkList();
            for (int placeMarkIndex = 0; placeMarkIndex < placemarkList.size(); ++placeMarkIndex)
            {
                if  (placemarkList.at(placeMarkIndex) == placemark)
                {
                    folder->remove(placeMarkIndex);
                    break;
                }
            }

        }
    }

    // Remove placemarks from lower level maps
    points.remove(index);
    lines.remove(index);
    lineHighlights.remove(index);
    angles.remove(index);
    angleHighlights.remove(index);
    angleReferenceLines.remove(index);
    angleReferenceLineHighlights.remove(index);
    ellipses.remove(index);
    ellipseVerticalBars.remove(index);
    rectangles.remove(index);
    rectangleVerticalBars.remove(index);

    // Remove appropriate references in indexToPlacemarkMap and placemarkToIndexMap
    indexToPlacemarkMap.remove(index);

    foreach(GeoDataPlacemark* placemark, placemarksToRemove)
    {
        placemarkToIndexMap.remove(placemark);
    }

    // If we are removing a position, remove it from pointIndexMap and finalPointMap
    if (isPointPlacemark)
    {
        QString label = QString::fromStdString(lsaRecord->getLabel());
        pointIndexMap.remove(label);
    }

    if (lsaType == LSAType::DGRP)
    {
        QString label = QString::fromStdString(lsaRecord->getLabel());
        dirGroupIndexMap.remove(label);
    }

    return;
}

void Mapping::insertPlacemarkFromIndex(QModelIndex index)
{
    if (!index.isValid())
        return;

    if (!guiModel->isActive(index))
        return;

    LSARecord* lsaRecord = guiModel->getLSARecord(index);
    LSAType lsaType = lsaRecord->getRecType();

    if     ((lsaType == LSAType::ENUO) ||
            (lsaType == LSAType::MEAN)) addDerived(index);
    else if (lsaType == LSAType::POSG)  addPOSG(index);
    else if (lsaType == LSAType::POSC)  addPOSC(index);
    else if (lsaType == LSAType::DIST)  addDIST(index);
    else if (lsaType == LSAType::DXYZ)  addDXYZ(index);
    else if (lsaType == LSAType::HANG)  addHANG(index);
    else if (lsaType == LSAType::AZIM)  addAZIM(index);
    else if (lsaType == LSAType::ZANG)  addZANG(index);
    else if (lsaType == LSAType::HDIF)  addHDIF(index);
    else if (lsaType == LSAType::HDIR)  addHDIR(index);
    else if (lsaType == LSAType::VANG)  addVANG(index);
    else if (lsaType == LSAType::DGRP)  addDGRP(index);
}

void Mapping::initialize()
{
    updateStyles();

    initModel();

    // Make sure selected points are highlighted
    handleSelectionChanged(QItemSelection(), QItemSelection());

    // Make sure disabled points are hidden
    updatePlacemarkVisibility();

    return;
}

void Mapping::disableMap()
{
    if (!mapIsEnabled) return;

    QSettings qsettings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    bool allowMapAutodisable = qsettings.value(lsa::QSETTINGS_ALLOW_AUTO_DISABLE_MAP, true).toBool();

    if (allowMapAutodisable)
    {
        QString message("Warning - slow gui response detected. Map is now auto-disabled to improve performance.");
        emit pushWarning(message);

        handleEnableDisableClicked();
    }

    return;
}

bool Mapping::sortManner(GeoDataFeature *lhPlacemark, GeoDataFeature *rhPlacemark)
{
    return rhPlacemark->name() > lhPlacemark->name();
}

void Mapping::sortPointsInFolders()
{
#ifdef _WIN32
    std::sort(POSGFolder->begin(), POSGFolder->end(), Mapping::sortManner);
    std::sort(POSCFolder->begin(), POSCFolder->end(), Mapping::sortManner);
    std::sort(meanFolder->begin(), meanFolder->end(), Mapping::sortManner);
    std::sort(enuoFolder->begin(), enuoFolder->end(), Mapping::sortManner);
#endif
}

void Mapping::handleSelectionChanged(QItemSelection selected, QItemSelection deselected)
{
    if (!mapIsEnabled) return;

    DebugTimer timer(false);

    updatePlacemarkHighlighting();
    LOG_TIME(timer, "updatePlacemarkHighlighting");

    redraw();
    LOG_TIME(timer, "redraw");
}

void Mapping::handleGuiModelRowsInserted(QModelIndex parentIndex, int first, int last)
{
    if (!mapIsEnabled) return;

    QTime procTimer;
    procTimer.start();

    Q_ASSERT(parentIndex.isValid());

    DebugTimer timer(false);
    // Add the new placemarks
    ui->map->model()->treeModel()->removeDocument(&mainDocument);

    LOG_TIME(timer, "Remove document")
    {
        int col = 0;
        for (int row = first; row <= last; ++row)
        {
            QModelIndex index = parentIndex.child(row,col);
            if (!index.isValid()) continue;

            // Insert a new placemark
            insertPlacemarkFromIndex(index);

            // If the new placemark is a position, update any placemarks that reference it
            updatePlacemarksReferencingPosition(index);
        }
        LOG_TIME(timer, "Add the new placemarks")
    }

    ui->map->model()->treeModel()->addDocument(&mainDocument);
    LOG_TIME(timer, "Add document")


    updatePlacemarkVisibility();
    LOG_TIME(timer,"Update placemark visibility");
    if ((points.size() > 0) && (lastPointsSize == 0))
    {
        setHome();
    }
    else
    {
        setHomeKeepCurrentView();
    }
    lastPointsSize = points.size();
    LOG_TIME(timer, "Set home");

    int procTime = procTimer.elapsed();
    if (procTime > HANDLE_ROW_INSERTED_TIMEOUT_MS ) disableMap();

    return;
}

void Mapping::handleGuiModelRowsAboutToBeRemoved(QModelIndex parentIndex, int first, int last)
{
    if (!mapIsEnabled) return;

    QTime procTimer;
    procTimer.start();

    // Remove the placemarks for the indexes about to be removed
    ui->map->model()->treeModel()->removeDocument(& mainDocument);
    {
        int col = 0;
        for (int row = first; row <= last; ++row)
        {
            QModelIndex index = guiModel->index(row, col, parentIndex);
            removePlacemark(index);
            updatePlacemarksReferencingPosition(index);
        }
    }
    ui->map->model()->treeModel()->addDocument(&mainDocument);

    updatePlacemarkVisibility();
    setHomeKeepCurrentView();

    int procTime = procTimer.elapsed();
    if (procTime > HANDLE_ROW_REMOVED_TIMEOUT_MS ) disableMap();

    return;
}

void Mapping::handleGuiModelDataChanged()
{
    if (!mapIsEnabled) return;

    QTime procTimer;
    procTimer.start();

    DebugTimer timer(false);

    initPlacemarks();

    LOG_TIME(timer, "initPlacemarks");

    if (points.size() == 1)
    {
        setHome();
    }
    else
    {
        setHomeKeepCurrentView();
    }
    LOG_TIME(timer, "setHomeKeepCurrentView");
    updatePlacemarkHighlighting();
    LOG_TIME(timer, "updatePlacemarkHighlighting");
    updatePlacemarkVisibility();
    LOG_TIME(timer, "updatePlacemarkVisibility");

    LOG_TIME(timer, "entire method");
    int procTime = procTimer.elapsed();
    if (procTime > HANDLE_DATA_CHANGED_TIMEOUT_MS ) disableMap();

    return;
}

void Mapping::updatePlacemarksForIndex(QModelIndex index)
{
    if (!index.isValid())
    {
        return;
    }

    // Delete the old placemarks for this index
    removePlacemark(index);

    // Create new updated placemarks for this index
    insertPlacemarkFromIndex(index);

    // If the new placemark is a position, update any placemarks that reference it
    updatePlacemarksReferencingPosition(index);

    // If this is an INCLUDE record, update all of its descendants too
    LSARecord *lsaRecord = guiModel->getLSARecord(index);
    if (lsaRecord != NULL)
    {
        if (lsaRecord->getRecType() == LSAType::INCLUDE)
        {
            QModelIndex parentIndex = index;
            QModelIndex currentIndex = guiModel->getNextDescendant(parentIndex, parentIndex);
            while(currentIndex.isValid())
            {
                updatePlacemarksForIndex(currentIndex);
                currentIndex = guiModel->getNextDescendant(parentIndex, currentIndex);
            }
        }
    }

    return;
}

void Mapping::updatePlacemarksReferencingPosition(QModelIndex index)
{
    // If this index is for an active(visible) POSG or POSC record, update all the measurements that reference it
    // We also need to update ellipses for MEAN and ENUO records
    LSAType lsaType = guiModel->getLSARecord(index)->getRecType();
    if ( lsaType == LSAType::POSC || lsaType == LSAType::POSG )
    {
        std::string positionLabel = guiModel->getLSARecord(index)->getLabel();

        // if this point is active, update every measurement that references it
        QModelIndex currentIndex = guiModel->getNextIndex();
        while(currentIndex.isValid())
        {
            LSARecord *lsaRecord = guiModel->getLSARecord(currentIndex);
            if (guiModel->recordReferencesPosition(lsaRecord,positionLabel))
            {
                updatePlacemarksForIndex(currentIndex);
            }

            currentIndex = guiModel->getNextIndex(currentIndex);
        }
    }

    return;
}

QPixmap Mapping::mapScreenShot()
{
    return ui->map->grab();
}

void Mapping::exportKML(QString kmzFileName, AdjustedPositions *adjustedPositions)
{
    sortPointsInFolders();
    redrawEllipses(true);
    redrawRectangles(true);

    // Remove highlights folder so it isn't exported into the KML file
    ui->map->model()->treeModel()->removeDocument(&mainDocument);
    int highlightsFolderIndex = mainDocument.size() - 1;
    GeoDataFeature *tempHighlightsFeature = mainDocument.child(highlightsFolderIndex);
    mainDocument.remove(highlightsFolderIndex); // Highlights folder should always be the last index in main document

    QFileInfo kmzFile(kmzFileName);

    // Create the KMZ directory structure
    QString kmzDirName = "__TMP__" + kmzFile.completeBaseName() + "__" + QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss");
    createKMZDir(kmzDirName);

    // Set the camera range for Google Earth
    GeoDataLookAt lookAt = ui->map->lookAt();
    qreal range = lookAt.range();
    lookAt.setRange(range*2.25); // Multiplied by a constant that results in a good view in Google Earth
    mainDocument.setAbstractView(&lookAt);

    // Add labels to the measurements and ellipse parts
    addLabelsKML();
    // Only add descriptions to points if exporting the adjusted coordinates
    if (!currentlyShowingInitialCoordinates)
    {
        QString description ="From salsa on " + QDate::currentDate().toString("MM/dd/yyyy") + "\n";
        QString geoid = "Geoid Used: " + adjustedPositions->getGeoid();
        description.append(geoid);
        mainDocument.setDescription(description);
        addDescriptionsToPoints(adjustedPositions);
    }

    // Export the kml file
    QString kmlFileName = kmzFile.completeBaseName() + ".kml";
    QString filePath = QDir::cleanPath(kmzDirName + "/" + QFileInfo(kmlFileName).fileName());
    ui->map->model()->fileManager()->saveFile(filePath, &mainDocument);

    // Add the correct paths for the embedded icons
    updateIconPathsKML(filePath);

    // Zip and remove the KMZ folder, leaving only the zipped file
    JlCompress::compressDir(kmzFileName, kmzDirName);
    int timesAttempted = 0;
    bool success = false;
    // If there was an error in removing the temp folder, wait 100ms before trying again.
    // Attempt to delete the folder a max of 3 times.
    do {
        success = QDir(kmzDirName).removeRecursively();
        ++timesAttempted;
        QTimer *timer = new QTimer(this);
        timer->start(100);
        while(timer->remainingTime() > 0);
    } while(!success && timesAttempted < 3);

    // Add the highlights folder back into kml for marble use
    // Dynamically cast geodatafeature back to geodatafolder
    GeoDataFolder *tempHighlightsFolder = dynamic_cast<GeoDataFolder*>(tempHighlightsFeature);
    if(tempHighlightsFolder){
        mainDocument.append(tempHighlightsFolder);
    }
    ui->map->model()->treeModel()->addDocument(&mainDocument);

    redrawEllipses();
    redrawRectangles();

    resetLabels();
    resetDescriptionsForPoints();
}

void Mapping::addLabelsKML()
{
    // Add labels for the KML
    foreach(GeoDataFolder* ellipses, ellipseFolder->folderList())
    {
        GeoDataPlacemark* ellipse = (GeoDataPlacemark*)ellipses->child(0);
        ellipse->setName("Ellipsoid");

        GeoDataPlacemark* vert = (GeoDataPlacemark*)ellipses->child(1);
        vert->setName("Ellipsoid Height");
    }

    foreach(GeoDataFolder* rectangles, rectangleFolder->folderList())
    {
        GeoDataPlacemark* rectangle = (GeoDataPlacemark*)rectangles->child(0);
        rectangle->setName("Reliability Rectangle");
        GeoDataPlacemark* vertRR = (GeoDataPlacemark*)rectangles->child(1);
        vertRR->setName("Rectangle Height");

    }

    foreach(GeoDataFolder* type, measurementFolder->folderList())
    {
        foreach(GeoDataFeature* measurement, type->featureList())
        {
            GeoDataPlacemark* vert = (GeoDataPlacemark*)measurement;
            QModelIndex index = placemarkToIndexMap.value(vert);
            std::string label = guiModel->getLSARecord(index)->asTypeString() + " - " + guiModel->getLSARecord(index)->getUIName();
            vert->setName(QString::fromStdString(label));
        }
    }
}

void Mapping::addDescriptionsToPoints(AdjustedPositions* adjustedPositions)
{

    QStandardItemModel* pointsModel = adjustedPositions->getPointsModel();
    // Add labels for the KML
    foreach(GeoDataFolder* points, pointsFolder->folderList())
    {
        // Points folder also contains the ellipse and rectangle folder. We only want descriptions for the points.
        if (points->name() != "Ellipse" || points->name() != "Rectangle" )
        {
            int pointsSize = points->placemarkList().size();
            for (int child = 0; child < pointsSize; ++child)
            {
                GeoDataPlacemark* point = (GeoDataPlacemark*)points->child(child);
                QString description = generateDescriptionText(adjustedPositions, pointsModel, point);
                point->setDescription(description);
            }
        }
    }

    foreach(GeoDataFolder* derivedPoints, derivedPointsFolder->folderList())
    {
        int derivedChildSize = derivedPoints->placemarkList().size();
        for(int derivedChild = 0; derivedChild < derivedChildSize; ++derivedChild)
        {
            GeoDataPlacemark* point = (GeoDataPlacemark*)derivedPoints->child(derivedChild);
            QString description = generateDescriptionText(adjustedPositions, pointsModel, point);
            point->setDescription(description);
        }

    }

}

QString Mapping::generateDescriptionText(AdjustedPositions * adjustedPositions, const QStandardItemModel *pointsModel, const GeoDataPlacemark *specPointFolder) const
{
    QString label = specPointFolder->name();
    QString latitude = "Latitude: ";
    QString longitude = "Longitude: ";
    QString eHt = "Ellip Ht: ";
    QString oHt = "Ortho Ht: ";
    QString n = "N: ";
    QString e = "E: ";
    QString u = "U: ";
    int rows = pointsModel->rowCount();
    for (int row = 0; row < rows; ++row)
    {
        QString pointsLabel = pointsModel->data(pointsModel->index(row, 0), Qt::DisplayRole).toString();
        if (pointsLabel == label)
        {
            latitude.append(pointsModel->data(pointsModel->index(row, 1), Qt::DisplayRole).toString());
            longitude.append(pointsModel->data(pointsModel->index(row, 2), Qt::DisplayRole).toString());

            eHt.append(QString::number(pointsModel->data(pointsModel->index(row, 3), Qt::DisplayRole).toDouble(),
                                         'f', getLinearPositionPrecisionMeters()));

            oHt.append(QString::number(pointsModel->data(pointsModel->index(row, 4), Qt::DisplayRole).toDouble(),
                                         'f', getLinearPositionPrecisionMeters()));

            llhExternal point = adjustedPositions->getPoint(label);
            n.append(QString::number(point.sigN, 'f', getLinearPositionPrecisionMeters()));

            e.append(QString::number(point.sigE, 'f', getLinearPositionPrecisionMeters()));

            u.append(QString::number(point.sigU, 'f', getLinearPositionPrecisionMeters()));

            break;
        }
    }
    QString description = latitude + "\n" + longitude + "\n" + eHt + " m\n" + oHt + " m\n"
                         + n + " m\n" + e + " m\n" + u + " m\n";

    return description;
}

void Mapping::resetDescriptionsForPoints()
{
    // Remove descriptions for SALSA
    foreach(GeoDataFolder* points, pointsFolder->folderList())
    {
        if (points->name() != "Ellipse" || points->name() != "Rectangle")
        {
            for (int child = 0; child < points->placemarkList().size(); ++child)
            {
                GeoDataPlacemark* point = (GeoDataPlacemark*)points->child(child);
                point->setDescription("");
            }
        }
    }

}

void Mapping::resetLabels()
{
    // Remove labels for SALSA
    foreach(GeoDataFolder* ellipses, ellipseFolder->folderList())
    {
        GeoDataPlacemark* ellipse = (GeoDataPlacemark*)ellipses->child(0);
        GeoDataPlacemark* vert = (GeoDataPlacemark*)ellipses->child(1);
        ellipse->setName("");
        vert->setName("");
    }

    foreach(GeoDataFolder* rectangles, rectangleFolder->folderList())
    {
        GeoDataPlacemark* rectangle = (GeoDataPlacemark*)rectangles->child(0);
        GeoDataPlacemark* vertRR = (GeoDataPlacemark*)rectangles->child(1);

        rectangle->setName("");
        vertRR->setName("");
    }

    foreach(GeoDataFolder* type, measurementFolder->folderList())
    {
        foreach(GeoDataFeature* measurement, type->featureList())
        {
            GeoDataPlacemark* vert = (GeoDataPlacemark*)measurement;
            vert->setName("");
        }
    }
}

void Mapping::createKMZDir(QString kmzDirName)
{
    // Create the kmz directory structure
    QDir().mkdir(kmzDirName);

    QDir resources( qApp->applicationDirPath() + "/../resources/mapIcons");

    QString resourcesDirName = QDir::cleanPath(kmzDirName + "/resources/mapIcons");
    QDir().mkpath(resourcesDirName);

    QFileInfoList list = resources.entryInfoList();

    // Copy SALSA's map icons into the KMZ folder
    foreach (QFileInfo file, list)
    {
        if (file.suffix() == "png")
        {
            QString currFileName = file.fileName();
            QString newFileName = QDir::cleanPath(resourcesDirName + "/" + currFileName);
            QFile(file.absoluteFilePath()).copy(newFileName);
        }
    }
}

void Mapping::updateIconPathsKML(QString filePath)
{
    // Read the KML file
    QFile kmlFile(filePath);
    kmlFile.open(QIODevice::ReadWrite);
    QByteArray data = kmlFile.readAll();
    QString text(data);

    // Replace QT resource paths with relative paths, make sure we are only replacing the icon paths
    text.replace(QRegExp("(<Style>\\s*<IconStyle>\\s*\\t*<Icon>\\s*<href>):/"), QString("\\1resources/"));

    // Replace the Fixed points path with the Google earth replica icon and set scale
    text.replace(QRegExp("(<Style>\\s*<IconStyle>)(\\s*\\t*<Icon>\\s*<href>)resources/mapIcons/fixed_all.png"),
                 QString("\\1<scale>0.7</scale>\\2resources/mapIcons/fixedPointTriangle.png"));

    // Replace the Floating points path with the Google earth replica icon and set scale
    text.replace(QRegExp("(<Style>\\s*<IconStyle>)(\\s*\\t*<Icon>\\s*<href>)resources/mapIcons/floating.png"),
                 QString("\\1<scale>0.5</scale>\\2resources/mapIcons/floatingPointCircle.png"));

    kmlFile.seek(0); // Go back to the beginning of the file
    kmlFile.write(text.toUtf8()); // Write the udpated text back to the KML file

    kmlFile.close();
}

void Mapping::setHome(bool openingProject)
{
    // FIXME: workaround the label/icon issue by reloading the map layer
    // This is a workaround to fix the following bugs:
    // Bug 1010 - Calculating Adjustment incorrectly jumbles point labels/icons
    // Bug 998 - TP point icons and labels rendered incorrectly when alternating between "Initial" and "Adjustment" views
    refreshMapLayer();

    QTime procTimer;
    procTimer.start();

    // this is the maximum zoom allowed for one or fewer visible points
    int homeZoomLevel = 2840;

    bool oldState = suppressGuiUpdates(true);
    {
        qreal homeLon, homeLat; //the center of the viewbox that will contain the network

        // Get a vector of all the visible points
        QVector<GeoDataPlacemark*> visiblePoints;
        foreach(GeoDataPlacemark* point, points)
        {
            QModelIndex index = placemarkToIndexMap.value(point);
            if (!index.isValid())
                continue;

            LSAType lsaType = guiModel->getLSARecord(index)->getRecType();
            if (isTypeVisible(lsaType))
                visiblePoints.push_back(point);
        }

        // Find the min and max lat and lon
        qreal maxLon, minLon, maxLat, minLat;
        QVector<qreal> lons;
        QVector<qreal> lats;
        if (visiblePoints.size() > 0)
        {
            maxLon = minLon = visiblePoints[0]->coordinate().longitude(GeoDataCoordinates::Degree);
            maxLat = minLat = visiblePoints[0]->coordinate().latitude(GeoDataCoordinates::Degree);

            foreach(GeoDataPlacemark* point, visiblePoints)
            {
                qreal lon = point->coordinate().longitude(GeoDataCoordinates::Degree);
                qreal lat = point->coordinate().latitude(GeoDataCoordinates::Degree);
                lons.push_back(lon);
                lats.push_back(lat);
                if (lon > maxLon) maxLon = lon;
                if (lon < minLon) minLon = lon;
                if (lat > maxLat) maxLat = lat;
                if (lat < minLat) minLat = lat;
            }
            homeLon = (maxLon + minLon) / 2.0;
            homeLat = (maxLat + minLat) / 2.0;

            // Starting at the maximum zoom level, check if all of the points are visible using the output of ui->map->screenCoordinates()
            // If they are all visible, stop because home is set correctly.
            // If the points are far apart (across the globe, for example) the loop will stop at the min zoom level
            int zoomStep = 100;
            homeZoomLevel = visiblePoints.size() < 2 ? homeZoomLevel : ui->map->maximumZoom();
            int homeZoom = homeZoomLevel + zoomStep;
            bool allPointsAreVisible = false, minLatLonVisible = false, maxLatLonVisible = false;
            qreal dummyX, dummyY; // not used

            suppressZoomUpdates = true;
            {
                while (!allPointsAreVisible)
                {
                    homeZoom -= zoomStep;

                    // Fix for bug 1449
                    if (homeLat <= -90)
                        homeLat = -89.9;
                    else if (homeLat >= 90 )
                        homeLat = 89.9;

                    ui->map->setZoom(homeZoom); // Fix needed for marble 23.04 upgrade

                    ui->map->model()->setHome(homeLon, homeLat, homeZoom);
                    ui->map->goHome();
                    if (homeZoom - zoomStep <= ui->map->minimumZoom())
                    {
                        break;
                    }

                    // Check if all points are visible
                    minLatLonVisible = ui->map->screenCoordinates(minLon, minLat, dummyX, dummyY);
                    maxLatLonVisible = ui->map->screenCoordinates(maxLon, maxLat, dummyX, dummyY);
                    allPointsAreVisible = ( minLatLonVisible && maxLatLonVisible );
                }
            }
            suppressZoomUpdates = false;
        }
        else
        {
            qreal defaultLon = 0;
            qreal defaultLat = 0;
            int maxZoom = homeZoomLevel;
            ui->map->model()->setHome(defaultLon, defaultLat, maxZoom);
        }
    }
    suppressGuiUpdates(oldState);
    ui->map->goHome();

    if (procTimer.elapsed() > SET_HOME_TIMEOUT_MS)
    {
        disableMap();
    }

    if(openingProject)
    {
        redrawEllipses();//Fix to Bug #1202
        redrawRectangles(true);
    }

    // Fix to bugs 1462 and 1463
    refreshMapLayer();

    return;
}

double Mapping::calculateEllipseScaling()
{
    return calculateEllipseScaling(getMaxAxisLengthMeters());
}

double Mapping::calculateEllipseScaling(double maxAxisLengthMeters)
{
    double degLatPerPixel = getDegLatPerPixel();
    double mapDiagonalPixels = getMapDiagonalPixels();

    double maxAxisLengthDegrees = degLatPerPixel * mapDiagonalPixels * currentEllipseFractionOfDiagonal;

    if(ui->checkEllipseAutoscale->isChecked())
    {
        // Set Values on the slider
        // The value of ui->sliderEllipseScale sets the size of the largest ellipse on the screen.
        // minValue = 0.  When set to zero, no ellipses should be visible on the map.
        // maxValue = NUM_SLIDER_DIVISIONS.  When set to max, the major axis of the largest ellipse
        // should be the size of the map diagonal on the screen.

        ui->sliderEllipseScale->setMaximum(NUM_SLIDER_DIVISIONS);
        ui->sliderEllipseScale->setValue(qCeil(currentEllipseFractionOfDiagonal * NUM_SLIDER_DIVISIONS));
        ui->sliderEllipseScale->setTracking(true); // slider will constantly emit valueChanged signal as user moves it

        double metersPerPixel = maxAxisLengthMeters / (getMapDiagonalPixels() * currentEllipseFractionOfDiagonal);
        ui->doubleEllipseScale->setValue(metersPerPixel * SCALE_BAR_WIDTH_PIXELS);
        ui->doubleEllipseScale->setSingleStep(metersPerPixel * SCALE_BAR_WIDTH_PIXELS / NUM_SLIDER_DIVISIONS);
    }

    return maxAxisLengthDegrees;
}

void Mapping::setHomeKeepCurrentView()
{
    DebugTimer timer(false);

    bool oldState = suppressGuiUpdates(true);
    qreal lon = ui->map->centerLongitude();
    qreal lat = ui->map->centerLatitude();
    qreal zoom = ui->map->zoom();
    LOG_TIME(timer, "get zoom settings");

    setHome();
    LOG_TIME(timer, "Set home");

    ui->map->setCenterLatitude(lat);
    ui->map->setCenterLongitude(lon);
    ui->map->setZoom(zoom);

    suppressGuiUpdates(oldState);
    LOG_TIME(timer, "set zoom settings");
}

void Mapping::redraw()
{
    if (!mapIsEnabled) return;

    ui->map->model()->treeModel()->removeDocument(&mainDocument);
    ui->map->model()->treeModel()->addDocument(&mainDocument);
}

void Mapping::lmbHandler(qreal lonClicked, qreal latClicked, Marble::GeoDataCoordinates::Unit unit)
{
    // Is a point being moved?  If so, treat the mouse click as specifying the new location for the point and
    // set coordinate (lonClicked, latClicked) on point's LSARecord
    if (isPointMoving && movingPlacemark)
    {
        QModelIndex index = placemarkToIndexMap[movingPlacemark];
        LSARecord *lsaRecord = guiModel->getLSARecord(index);

        // convert to degrees
        if (unit == GeoDataCoordinates::Radian)
        {
            lonClicked *= RAD2DEG;
            latClicked *= RAD2DEG;
        }

        // if the record is POSG
        if (lsaRecord->getRecType() == LSAType::POSG)
        {
            LSAPosG* point = static_cast<LSAPosG*>(lsaRecord);
            if (point->useDecimalDegrees)
            {
                point->latDecDeg = latClicked;
                point->lonDecDeg = lonClicked;
                point->latDir = "N";
                point->lonDir = "W";
            }
            else
            {
//                int round = 10; // How many decimal places...I don't know what this should be so I chose a large number

                int latDegrees;
                int latMinutes;
                double latSeconds;
                bool is_lat_neg;

                degToDMS(latClicked, is_lat_neg, latDegrees, latMinutes, latSeconds);

                int lonDegrees;
                int lonMinutes;
                double lonSeconds;
                bool is_lon_neg;

                degToDMS(lonClicked, is_lon_neg, lonDegrees,lonMinutes, lonSeconds);

                // Point conventions come from marble using NE positive
                point->latDir = "N";
                if(is_lat_neg)
                {
                    point->latDir = "S";
                }
                point->latDeg = latDegrees;
                point->latMin = latMinutes;
                point->latSec = latSeconds;
//                point->latSec = roundDoubleToPrecision(latSeconds,10);

                point->lonDir = "E";
                if(is_lon_neg)
                {
                    point->lonDir = "W";
                }
                point->lonDeg = lonDegrees;
                point->lonMin = lonMinutes;
                point->lonSec = lonSeconds;
//                point->lonSec = roundDoubleToPrecision(lonSeconds,10);
            }

        }
        // if the record is POSC use gnsstk to convert (lonClicked, latClicked) to cartesian
        else if (lsaRecord->getRecType() == LSAType::POSC)
        {
            LSAPosC* point = static_cast<LSAPosC*>(lsaRecord);

            // get the current altitude of the point, we only want to change its lat and lon
            gnsstk::Position oldPosition(point->x,point->y,point->z);
            oldPosition = oldPosition.asGeodetic();

            double height = oldPosition.getAltitude();

            gnsstk::Position newPosition(latClicked, lonClicked, height, gnsstk::Position::Geodetic);

            newPosition = newPosition.transformTo(gnsstk::Position::Cartesian);

            point->x = newPosition.X();
            point->y = newPosition.Y();
            point->z = newPosition.Z();

        }

        // emit a signal so that MainWindow knows it needs to update things
        // this probably calls update() on the map
        emit pointCoordinatesChanged(index);

        // we are no longer moving a point
        isPointMoving = false;
        movingPlacemark = NULL;

        modelIsSaved = false;
        ui->map->setMouseTracking(true);

        // We init the map model here to fix a bug that can cause GeoDataPlacemarks to not be deleted from the map correctly
        // after a user right clicks and moves a point.
        initModelPreserveZoom();
    }
    else
    {
        // otherwise this is not a move event, see if selection needs to be updated if the user clicked near a placemark
        const QPointF centerPoint = pointFromCoordinate(GeoDataCoordinates(lonClicked, latClicked, DEFAULT_HEIGHT, unit));
        updateSelection(centerPoint);
    }
}

void Mapping::updateSelection(const QPointF &centerPoint)
{
    // Use findAllNear() to generate a list of placemarks that are "near" enough to the given point on the screen
    // The second boolean argument of this function call specifies that points should be given priority;
    // in other words if a point is near the given click location only points will appear in selectionCandidates.
    QMap<QModelIndex, GeoDataPlacemark*> selectionCandidates = findAllNear(centerPoint, true);

    if (selectionCandidates.size() == 1)
    {
        // only one placemark to select
        QModelIndex index = selectionCandidates.keys().at(0);
        guiModel->select(index);
    }
    else if (selectionCandidates.size() > 1)
    {
        // multiple placemarks could have been selected, display a menu to the user to let them chose one
        QMenu myMenu;
        QMap<QAction*, GeoDataPlacemark*> actionToPlacemark;

        foreach(QModelIndex index, selectionCandidates.keys() )
        {
            if (index.isValid())
            {
                LSARecord* record = guiModel->getLSARecord(index);
                if(isTypeVisible(record->getRecType()) && guiModel->isActive(index))
                {
                    QString actionName = QString::fromStdString(record->asTypeString() + " - " + record->getUIName());
                    QAction* action = myMenu.addAction(actionName);
                    actionToPlacemark.insert(action, selectionCandidates[index]);
                }
            }

        }
        QAction* selectedItem = myMenu.exec(this->mapToGlobal(QPoint(qRound(centerPoint.x()),qRound(centerPoint.y()))));
        if (selectedItem)
        {
            QModelIndex index = placemarkToIndexMap[actionToPlacemark[selectedItem]];
            guiModel->select(index);
        }
    }
    redraw();  // necessary in order to get measurements to update if highlighting is changed in the above methods
}

//right click context menu slot
void Mapping::showContextMenu(const QPoint& pos)
{
    // don't show the context menu (allow moving points) in output mode
    if (ui->radioDisplayAdjusted->isChecked())
    {
        return;
    }

    // pos is in screen coordinates relative to Mapping, change this to screen coordinates relative to the map widget
    QPoint mapRelativePos = ui->map->mapFromGlobal(this->mapToGlobal(pos));
    QMap<QModelIndex, GeoDataPlacemark*> placemarks = findAllNear(mapRelativePos, true);
    if (placemarks.size() > 0)
    {
        QModelIndex index = placemarks.keys().at(0); // chose the top placemark clicked to be the one that is moved
        if ( index.isValid() )
        {
            LSARecord* lsaRecord = guiModel->getLSARecord(index);
            if (lsaRecord->getRecType() == LSAType::POSG && lsaRecord->isAutogenerated)
            {
                // only move the point if it is for an autogenerated POSG record
                QMenu myMenu;
                myMenu.addAction("Move Point");
                QAction* selectedItem = myMenu.exec(this->mapToGlobal(pos));
                if (selectedItem)
                {
                    movingPlacemark = placemarks.values().at(0);
                    isPointMoving = true;
                    ui->map->setMouseTracking(false);
                    // set cursor
                    // image file locations must be the same as in the constructor
                    ui->map->setCursor(QCursor(QPixmap(PATH_APRIORI)));
                }
            }
        }
    }
}

void Mapping::initMapInitialCoodinates()
{
    ui->radioDisplayInitial->setChecked(true);
    initModelPreserveZoom();
    currentlyShowingInitialCoordinates = true;
}

void Mapping::initMapAdjustedCoordinates()
{
    ui->radioDisplayAdjusted->setChecked(true);
    initModelPreserveZoom();
    currentlyShowingInitialCoordinates = false;
}

void Mapping::handleRadioInitialClicked()
{
    if (currentlyShowingInitialCoordinates) return;

    initModelPreserveZoom();
    currentlyShowingInitialCoordinates = true;
}

void Mapping::handleRadioAdjustedClicked()
{
    if (!currentlyShowingInitialCoordinates) return;

    initModelPreserveZoom();
    currentlyShowingInitialCoordinates = false;
}

void Mapping::initModelPreserveZoom()
{
    qreal lon = ui->map->centerLongitude();
    qreal lat = ui->map->centerLatitude();
    qreal zoom = ui->map->zoom();
    initModel();
    ui->map->setCenterLatitude(lat);
    ui->map->setCenterLongitude(lon);
    ui->map->setZoom(zoom);
}

void Mapping::handleCheckDisplayMapLayerClicked()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    settings.setValue( lsa::QSETTINGS_SHOW_MAP_CHECKED, ui->checkDisplayMapLayer->isChecked() );

    if (ui->checkDisplayMapLayer->isChecked())
    {
        if(ui->map->zoom() <= SCROLL_ZOOM_THRESHOLD)
        {
            showBackgroundMap();
        }
    }
    else
    {
        hideBackgroundMap();
    }
}


bool Mapping::isTypeVisible(LSAType lsaType)
{
    if      (lsaType == LSAType::POSG ||
             lsaType == LSAType::MEAN ||
             lsaType == LSAType::ENUO) return ui->checkDisplayPOSG->isChecked();
    else if (lsaType == LSAType::POSC) return ui->checkDisplayPOSC->isChecked();
    else if (lsaType == LSAType::DIST) return ui->checkDisplayDIST->isChecked();
    else if (lsaType == LSAType::DXYZ) return ui->checkDisplayDXYZ->isChecked();
    else if (lsaType == LSAType::AZIM) return ui->checkDisplayAZIM->isChecked();
    else if (lsaType == LSAType::HANG) return ui->checkDisplayHANG->isChecked();
    else if (lsaType == LSAType::VANG) return ui->checkDisplayVANG->isChecked();
    else if (lsaType == LSAType::ZANG) return ui->checkDisplayZANG->isChecked();
    else if (lsaType == LSAType::HDIF) return ui->checkDisplayHDIF->isChecked();
    else if (lsaType == LSAType::HDIR) return ui->checkDisplayHDIR->isChecked();

    return false;
}

void Mapping::updatePlacemarkVisibility()
{
    if (suppressMapWidgetUpdates) return;

    // Set the visibility for all placemarks
    for (QMap<GeoDataPlacemark*, QPersistentModelIndex>::iterator iter = placemarkToIndexMap.begin(); iter != placemarkToIndexMap.end(); ++iter)
    {
        QModelIndex           index = iter.value();
        if (index.isValid())
        {
            GeoDataPlacemark* placemark = iter.key();
            LSARecord*        lsaRecord = guiModel->getLSARecord(index);
            LSAType             lsaType = lsaRecord->getRecType();

            bool typeIsVisible = isTypeVisible(lsaType);
            bool recordIsActive = guiModel->isActive(index);
            placemark->setVisible(typeIsVisible && recordIsActive);
        }
    }

    redrawEllipses();
    redrawRectangles();

    return;
}

void Mapping::updatePlacemarkHighlighting()
{
    if (suppressMapWidgetUpdates) return;

    unHighlightAll();

    QModelIndexList indexList = guiModel->getSelectedRows();
    foreach (QModelIndex index, indexList)
    {
        if(guiModel->isActive(index))
        {
            QList<GeoDataPlacemark*> placemarks = indexToPlacemarkMap.values(index);
            foreach(GeoDataPlacemark* placemark, placemarks)
            {
                highlight(index, placemark); // TODO scj: only pass index here once conversion to maps is complete
            }
        }
    }

    return;
}

bool Mapping::suppressGuiUpdates(bool value)
{
    bool oldState = suppressMapWidgetUpdates;

    suppressMapWidgetUpdates = value;
    emit signalSuppressGuiUpdates(value);

    return oldState;
}

/**
 * @brief getVisualCategoryForType Given an LSAVisualCategoryOffset, return the correct
 * GeoDataVisualCategory. This is used directly for error ellipses and rectangles. Error
 * Ellipses and rectanges can't be selected.
 * @param type The LSAVisualCategoryOffset
 * @return The appropriate GeoDataVisualCategory after applying the LSAVisualCategoryOffset
 */
GeoDataPlacemark::GeoDataVisualCategory getVisualCategoryForType(LSAVisualCategoryOffset offset)
{
    LSAVisualCategory base = LSAVisualCategory::Default;
    return GeoDataPlacemark::GeoDataVisualCategory(base + offset);
}

/**
 * @brief getVisualCategoryForType Given a record type with a status, gives the appropriate Visual Category
 * for drawing with Marble. Each type is normally drawn one after another, though within a VC order isn't
 * guarenteed. If something is selected, its highlighting line is drawn after all unselected items, and then
 * the selected line itself is moved to be drawn after its highlighting line.
 * @param type The record type
 * @param status If the record is Selected or not (Default).
 * @return The appropriate Visual Category for a record of type in state status
 */
GeoDataPlacemark::GeoDataVisualCategory getVisualCategoryForType(LSAType type, HighlightStatus status)
{
    LSAVisualCategory base = LSAVisualCategory::Default;

    if(status == HighlightStatus::DEFAULT)
    {
        // line
        if(type == LSAType::DIST)
            return getVisualCategoryForType(LSAVisualCategoryOffset::DIST);
        else if(type == LSAType::DXYZ)
            return getVisualCategoryForType(LSAVisualCategoryOffset::DXYZ);
        else if(type == LSAType::ZANG)
            return getVisualCategoryForType(LSAVisualCategoryOffset::ZANG);
        else if(type == LSAType::HDIF)
            return getVisualCategoryForType(LSAVisualCategoryOffset::HDIF);
        else if(type == LSAType::HDIR)
            return getVisualCategoryForType(LSAVisualCategoryOffset::HDIR);
        else if(type == LSAType::VANG)
            return getVisualCategoryForType(LSAVisualCategoryOffset::VANG);
        // angle
        else if(type == LSAType::AZIM)
            return getVisualCategoryForType(LSAVisualCategoryOffset::AZIM);
        else if(type == LSAType::HANG)
            return getVisualCategoryForType(LSAVisualCategoryOffset::HANG);
    }
    else
    {
        return GeoDataPlacemark::GeoDataVisualCategory(LSAVisualCategory::Selected);
    }

    return GeoDataPlacemark::GeoDataVisualCategory(LSAVisualCategory::Default);
}


void Mapping::addPlacemark_Point(QString label, const GeoDataCoordinates &coordinate, const QModelIndex &index, IconType fixedStatus, HighlightStatus highlightStatus)
{
    // create a placemark
    GeoDataPlacemark *point = new GeoDataPlacemark(label);

    points.insert(index, point);

    // set the placemark's coordinate
    point->setCoordinate(coordinate);

    // set the bubble balloon to not visible
    point->setBalloonVisible(false);


    // ********************** CHANGE ICON FOR DERIVED POINTS ************************
    LSAType lsaType = guiModel->getLSARecord(index)->getRecType();
    bool currentlyDisplayingInput = ui->radioDisplayInitial->isChecked();

    //checking if the LSA type is MEAN or ENUO
    if(lsaType.isPostProcessed())
    {
        point->setStyle(styleDerived);
    }
    else
    {

        switch(highlightStatus)
        {
        case DEFAULT:
            switch(fixedStatus)
            {
            case POINT_FIXED_ALL:
                point->setStyle(styleFixedAll);
                break;
            case POINT_FIXED_1:
                point->setStyle(styleFixed1);
                break;
            case POINT_FIXED_2:
                point->setStyle(styleFixed2);
                break;
            case POINT_FLOATING:
                point->setStyle(styleFloating);
                break;
            case POINT_CONSTRAIN:
                point->setStyle(styleConstrain);
                break;
            case POINT_APRIORI:
                //initital or adjusted
                if(currentlyDisplayingInput)
                    point->setStyle(styleAPriori);
                else
                    point->setStyle(styleFloating);
                break;
            default:
                point->setStyle(styleFloating);
            }
            break;
        case HIGHLIGHTED:
            switch(fixedStatus)
            {
            case POINT_FIXED_ALL:
                point->setStyle(styleFixedAllHighlighted);
                break;
            case POINT_FIXED_1:
                point->setStyle(styleFixed1Highlighted);
                break;
            case POINT_FIXED_2:
                point->setStyle(styleFixed2Highlighted);
                break;
            case POINT_FLOATING:
                point->setStyle(styleFloatingHighlighted);
                break;
            case POINT_CONSTRAIN:
                point->setStyle(styleConstrainHighlighted);
                break;
            case POINT_APRIORI:
                //initial or adjusted
                if(currentlyDisplayingInput)
                    point->setStyle(styleAPrioriHighlighted);
                else
                    point->setStyle(styleFloating);
                break;
            default:
                point->setStyle(styleFloatingHighlighted);
            }
            break;
        default:
            break;
        }
    }

    if (lsaType == LSAType::POSG)
    {
        POSGFolder->append(point);
    }
    else if (lsaType == LSAType::POSC)
    {
        POSCFolder->append(point);
    }
    else if (lsaType == LSAType::MEAN)
    {
        meanFolder->append(point);
    }
    else if (lsaType == LSAType::ENUO)
    {
        enuoFolder->append(point);
    }
    else
    {
        Q_ASSERT(false);
    }

    placemarkToIndexMap.insert(point, QPersistentModelIndex(index));
    indexToPlacemarkMap.insert(QPersistentModelIndex(index), point);

    return;
}

void Mapping::addPlacemark_Line(const GeoDataCoordinates &from, const GeoDataCoordinates &to, const QModelIndex &index, HighlightStatus status)
{
    // create an object to hold the geometry of the line
    GeoDataLineString * lineString = new GeoDataLineString;

    // create a placemark
    GeoDataPlacemark * line = new GeoDataPlacemark();

    // create a placemark for the highlight line
    GeoDataPlacemark * highlight = new GeoDataPlacemark();
    highlight->setVisualCategory(GeoDataPlacemark::GeoDataVisualCategory(LSAVisualCategory::Highlight));

    lines.insert(index, line);
    placemarkToIndexMap.insert(line, index);
    lineHighlights.insert(index, highlight);

    // add points to the line
    lineString->append(from);
    lineString->append(to);

    // marble requires that each placemark refer to its own geometry, or it
    // segmentation faults when the GeoDataDocument "document" is cleared
    GeoDataLineString * lineStringHighlight = new GeoDataLineString(*lineString);

    // set the geometry of the placemark
    line->setGeometry(lineString);
    highlight->setGeometry(lineStringHighlight);

    // if the index isn't valid, use marble's default styles
    if (index.isValid())
    {
        LSAType lsaType = guiModel->getLSARecord(index)->getRecType();


        // Get the appropriate visual category for the item. They highlighting line should always be the same.
        line->setVisualCategory(getVisualCategoryForType(lsaType, status));
        // We don't want the default min zoom level that Marble's predefined Visual Category gives us.
        line->setZoomLevel(1);
        highlight->setZoomLevel(1);

        if (lsaType == LSAType::DIST)
        {
            line->setStyle(DISTStyle);
            highlight->setStyle(DISTStyleHighlighted);
            DISTFolder->append(line);
        }
        else if (lsaType == LSAType::DXYZ)
        {
            line->setStyle(DXYZStyle);
            highlight->setStyle(DXYZStyleHighlighted);
            DXYZFolder->append(line);
        }
        else if (lsaType == LSAType::ZANG)
        {
            line->setStyle(ZANGStyle);
            highlight->setStyle(ZANGStyleHighlighted);
            ZANGFolder->append(line);
        }
        else if (lsaType == LSAType::HDIF)
        {
            line->setStyle(HDIFStyle);
            highlight->setStyle(HDIFStyleHighlighted);
            HDIFFolder->append(line);
        }
        else if (lsaType == LSAType::HDIR)
        {
            line->setStyle(HDIRStyle);
            highlight->setStyle(HDIRStyleHighlighted);
            HDIRFolder->append(line);
        }
        else if (lsaType == LSAType::VANG)
        {
            line->setStyle(VANGStyle);
            highlight->setStyle(VANGStyleHighlighted);
            VANGFolder->append(line);
        }
        else
        {
            Q_ASSERT(false);
        }
    }

    if (status == HIGHLIGHTED)
    {
        highlight->setVisible(true);
    }
    else
    {
        highlight->setVisible(false);
    }
    highlightsFolder->append(highlight);

    placemarkToIndexMap.insert(line,index);
    indexToPlacemarkMap.insert(index, line);
}

void Mapping::addPlacemark_EllipseFromCovariance(std::string ellipseName, QModelIndex pointIndex, const GeoDataCoordinates &centerPosition, qreal covNN,
                                                 qreal covNE, qreal covNU, qreal covEE, qreal covEU, qreal covUU, qreal scaleDegreesPerMeter)
{
    //create a folder to holder both ellipse and vertuncr
    GeoDataFolder *ellipseAndVertUncr = new GeoDataFolder;
    ellipseAndVertUncr->setName(QString::fromStdString(ellipseName + "_Ellipse"));


    GeoDataPlacemark* ellipse = new GeoDataPlacemark;
    //apply user scale from preferences in the config file
    GeoDataLinearRing* ellipseGeometry = createEllipseFromCovariance(centerPosition, covNN, covNE, covNU, covEE, covEU, covUU, scaleDegreesPerMeter * ellipseConfidenceFactor2D);
    ellipse->setGeometry(ellipseGeometry);
    ellipse->setStyle(ellipseStyle);
    ellipse->setVisualCategory(getVisualCategoryForType(LSAVisualCategoryOffset::ELPS));
    ellipse->setZoomLevel(1);
    ellipses.insert(pointIndex, ellipse);
    ellipseAndVertUncr->append(ellipse);

    //create vertical error bar for ellipse
    GeoDataPlacemark* vertUncr = new GeoDataPlacemark;
    GeoDataLineString* vertUncrGeo = createEllipseHeightFromCovariance(centerPosition, covUU, scaleDegreesPerMeter);
    vertUncr->setGeometry(vertUncrGeo);
    vertUncr->setStyle(ellipseStyle);
    vertUncr->setVisualCategory(getVisualCategoryForType(LSAVisualCategoryOffset::ELPS));
    vertUncr->setZoomLevel(1);
    ellipseVerticalBars.insert(pointIndex, vertUncr);
    ellipseAndVertUncr->append(vertUncr);

    //add folder to ellipseFolder
    ellipseFolder->append(ellipseAndVertUncr);
}

void Mapping::addPlacemark_Rectangle(std::string rectangleName, QModelIndex pointIndex, const GeoDataCoordinates &centerPosition, qreal maj2Drr,
                                     qreal min2Drr, qreal verticalRR, qreal angle, qreal scaledDegreesPerMeter)
{
    //create a folder to holder both rectangle and vertrr
    GeoDataFolder *rectangleAndVertRR = new GeoDataFolder;
    rectangleAndVertRR->setName(QString::fromStdString(rectangleName + "_Rectangle"));

    GeoDataPlacemark* rectangle = new GeoDataPlacemark;
    //apply user scale from preferences in the config file
    GeoDataLinearRing* rectangleGeometry = createRectangle(centerPosition, maj2Drr, min2Drr, angle, scaledDegreesPerMeter);
    rectangle->setGeometry(rectangleGeometry);
    rectangle->setStyle(rectangleStyle);
    rectangle->setVisualCategory(getVisualCategoryForType(LSAVisualCategoryOffset::RECT));
    rectangle->setZoomLevel(1);
    rectangles.insert(pointIndex, rectangle);
    rectangleAndVertRR->append(rectangle);

    //create vertical error bar for ellipse
    GeoDataPlacemark* vertRR = new GeoDataPlacemark;
    GeoDataLineString* vertrr= createRectangleHeight(centerPosition, verticalRR, scaledDegreesPerMeter);
    vertRR->setGeometry(vertrr);
    vertRR->setStyle(rectangleStyle);
    vertRR->setVisualCategory(getVisualCategoryForType(LSAVisualCategoryOffset::RECT));
    vertRR->setZoomLevel(1);
    rectangleVerticalBars.insert(pointIndex, vertRR);
    rectangleAndVertRR->append(vertRR);

    //add folder to rectangleFolder
    rectangleFolder->append(rectangleAndVertRR);
}

void Mapping::addPlacemark_Angle(const GeoDataCoordinates &from, const GeoDataCoordinates &at, const GeoDataCoordinates &to, const QModelIndex &index, HighlightStatus status)
{

    if ( !index.isValid() ) return;

    GeoDataPlacemark* angle = new GeoDataPlacemark;
    GeoDataPlacemark* angleHighlight = new GeoDataPlacemark;
    GeoDataPlacemark* angleReferenceLine = new GeoDataPlacemark;
    GeoDataPlacemark* angleReferenceLineHighlight = new GeoDataPlacemark;


    GeoDataLineString* angleGeometry = createAngle(from,at,to);
    GeoDataLineString* angleHighlightGeometry = new GeoDataLineString(*angleGeometry);
    GeoDataLineString* angleReferenceLineGeometry = new GeoDataLineString;
    *angleReferenceLineGeometry << from << at << to; // the reference line between the points
    GeoDataLineString* angleReferenceLineHighlightGeometry = new GeoDataLineString(*angleReferenceLineGeometry);

    angle->setGeometry(angleGeometry);
    angleHighlight->setGeometry(angleHighlightGeometry);
    angleReferenceLine->setGeometry(angleReferenceLineGeometry);
    angleReferenceLineHighlight->setGeometry(angleReferenceLineHighlightGeometry);

    // if the index isn't valid, use marble's default styles
    LSAType angleType = guiModel->getLSARecord(index)->getRecType();

    // Get the appropriate visual category for the item. They highlighting line should always be the same.
    angle->setVisualCategory(getVisualCategoryForType(angleType, status));
    angle->setZoomLevel(1);
    angleHighlight->setVisualCategory(GeoDataPlacemark::GeoDataVisualCategory(LSAVisualCategory::Highlight));
    angleHighlight->setZoomLevel(1);
    angleReferenceLine->setVisualCategory(getVisualCategoryForType(angleType, status));
    angleReferenceLine->setZoomLevel(1);
    angleReferenceLineHighlight->setVisualCategory(GeoDataPlacemark::GeoDataVisualCategory(LSAVisualCategory::Highlight));
    angleReferenceLineHighlight->setZoomLevel(1);

    if (angleType == LSAType::AZIM)
    {

        angle->setStyle(AZIMStyle);
        angleHighlight->setStyle(AZIMStyleHighlighted);
        angleReferenceLine->setStyle(AZIMReferenceLineStyle);
        angleReferenceLineHighlight->setStyle(AZIMReferenceLineStyleHighlighted);
        AZIMFolder->append(angle);
        AZIMFolder->append(angleReferenceLine);
    }
//    else if (angleType == LSAType::HANG) //TODO: Same as the else statement
//    {
//        angle->setStyle(HANGStyle);
//        angleHighlight->setStyle(HANGStyleHighlighted);
//        angleReferenceLine->setStyle(HANGReferenceLineStyle);
//        angleReferenceLineHighlight->setStyle(HANGReferenceLineStyleHighlighted);
//    }
    else
    {
        angle->setStyle(HANGStyle);
        angleHighlight->setStyle(HANGStyleHighlighted);
        angleReferenceLine->setStyle(HANGReferenceLineStyle);
        angleReferenceLineHighlight->setStyle(HANGReferenceLineStyleHighlighted);
        HANGFolder->append(angle);
        HANGFolder->append(angleReferenceLine);
    }

    angles.insert(index, angle);
    placemarkToIndexMap.insert(angle, index);

    angleReferenceLines.insert(index, angleReferenceLine);
    placemarkToIndexMap.insert(angleReferenceLine, index);

    angleHighlights.insert(index, angleHighlight);
    angleReferenceLineHighlights.insert(index, angleReferenceLineHighlight);

    if (status == HIGHLIGHTED)
    {
        angleHighlight->setVisible(true);
        angleReferenceLineHighlight->setVisible(true);
    }
    else
    {
        angleHighlight->setVisible(false);
        angleReferenceLineHighlight->setVisible(false);
    }

    highlightsFolder->append(angleHighlight);
    highlightsFolder->append(angleReferenceLineHighlight);

    placemarkToIndexMap.insert(angle, index);
    placemarkToIndexMap.insert(angleReferenceLine, index);

    indexToPlacemarkMap.insert(index, angle);
    indexToPlacemarkMap.insert(index, angleReferenceLine);
}

void Mapping::handleZoomChange()
{
    if(previousZoomLevel != ui->map->zoom()) {
        if (!suppressZoomUpdates)
        {
            redrawEllipses();
            redrawRectangles();
            adaptiveZoomThemeHandler();
        }
    }
    previousZoomLevel = ui->map->zoom();
}

void Mapping::refreshMapLayer()
{
    // only change behavior if the checkbox is checked
    if(ui->checkDisplayMapLayer->isChecked())
    {
        if(ui->map->zoom() >= SCROLL_ZOOM_THRESHOLD)
        {
            highScrollZoomEnabled = true;
            showBackgroundMap();
            hideBackgroundMap();
        }
        else
        {
            highScrollZoomEnabled = false;
            hideBackgroundMap();
            showBackgroundMap();
        }
    }
    else
    {
        showBackgroundMap();
        hideBackgroundMap();
    }

}

void Mapping::adaptiveZoomThemeHandler()
{
    // only change behavior if the checkbox is checked
    if(ui->checkDisplayMapLayer->isChecked())
    {
        if(ui->map->zoom() >= SCROLL_ZOOM_THRESHOLD)
        {
            // only update the map once when threshold is exceeded
            if(!highScrollZoomEnabled)
            {
                highScrollZoomEnabled = true;
                hideBackgroundMap();

            }
        }
        else
        {
            // only update the map once when threshold is exceeded
            if(highScrollZoomEnabled)
            {
                highScrollZoomEnabled = false;
                showBackgroundMap();
            }
        }
    }
}

bool Mapping::redrawEllipses(bool exportingKML)
{
    // early returns
    if ( !guiModel ) return false;
    if ( !ui->radioDisplayAdjusted->isChecked() ) return false;
    if ( suppressMapWidgetUpdates ) return false;

    ui->map->model()->treeModel()->removeDocument(&mainDocument);

    // remove all of the old ellipses from mainDocument
    // this only matters when redrawEllipses is called between calls to update()
    QVector<GeoDataFolder*> folderList = ellipseFolder->folderList();
    foreach( GeoDataFolder *ellipseAndVertUncrFolder, folderList )
    {
        ellipseAndVertUncrFolder->clear();
        ellipseFolder->remove(ellipseFolder->childPosition(ellipseAndVertUncrFolder));
    }

    if (ui->checkEllipseAutoscale->isChecked())
    {
        currentEllipseFractionOfDiagonal = DEFAULT_FRACTION_OF_DIAGONAL;
    }

    if (ui->radioDisplayAdjusted->isChecked())
    {
        // find the max axis length of all ellipses
        double maxAxisLengthMeters = getMaxAxisLengthMeters();
        double maxAxisLengthDegrees = calculateEllipseScaling(maxAxisLengthMeters);

        // add ellipses
        if(maxAxisLengthMeters > ELLIPSE_MIN_RADIUS_THRESHOLD)
        {
            foreach (const FinalAdjustedPosition& point, finalPointMap)
            {
                QModelIndex pointIndex = pointIndexMap.value(QString::fromStdString(point.position));

                if (pointIndex.isValid() && guiModel->isActive(pointIndex))
                {
                    LSARecord* lsaRecord = guiModel->getLSARecord(pointIndex);
                    LSAType    lsaType = lsaRecord->getRecType();
                    bool typeIsVisible = isTypeVisible(lsaType);
                    bool pointIsFixed = true;
                    if (lsaType == LSAType::POSG)
                    {
                        LSAPosG* lsaPoint = static_cast<LSAPosG*>(lsaRecord);
                        pointIsFixed = lsaPoint->fixedState == LSAFixedState::FIXED;
                    }
                    else if (lsaType == LSAType::POSC)
                    {
                        LSAPosC* lsaPoint = static_cast<LSAPosC*>(lsaRecord);
                        pointIsFixed = lsaPoint->fixedState == LSAFixedState::FIXED;
                    }
                    else if (lsaType == LSAType::MEAN || lsaType == LSAType::ENUO)
                    {
                        pointIsFixed = false;
                    }

                    // The commented code below calculates the physical, unscaled degrees per meter for the ellipses.
                    // It was originally used for KML Export to provide Google Earth with a physical representation
                    // of the ellipses and ellipsoid heights.
//                    gnsstk::WGS84Ellipsoid ellipse;
//                    double eccentricity_squared = ellipse.eccSquared();
//                    double semi_major_axis = ellipse.a();
//                    double lat = getCoordinatesFromLabel(point.position).latitude();
//                    double radius = semi_major_axis * (cos(lat))/(pow((1.0 - eccentricity_squared * pow(sin(lat), 2)), (0.5)));
//                    double unscaledDegreesPerMeter = RAD_TO_DEG/radius;
                    double scaledDegreesPerMeter = 0;
                    if(ui->checkDisplayEllipses->isChecked() && typeIsVisible)
                    {
                        scaledDegreesPerMeter = maxAxisLengthDegrees / maxAxisLengthMeters; // Used to convert ellipse sizes in meters to map plotting units of degrees
                    }
                    if (!pointIsFixed)
                    {
                        std::string ellipseName = lsaRecord->getLabel();
                        addPlacemark_EllipseFromCovariance(ellipseName, pointIndex, getCoordinatesFromLabel(point.position),
                                                           point.covNN, point.covNE, point.covNU,
                                                           point.covEE, point.covEU, point.covUU, scaledDegreesPerMeter);
                    }
                }
            }
        }
    }
    ui->map->model()->treeModel()->addDocument(&mainDocument);
    redrawRectangles();

    return true;
}

bool Mapping::redrawRectangles(bool exportingKML)
{
    // early returns
    if ( !guiModel ) return false;
    if ( !ui->radioDisplayAdjusted->isChecked() ) return false;
    if ( suppressMapWidgetUpdates ) return false;

    ui->map->model()->treeModel()->removeDocument(&mainDocument);

    // remove all of the old rectangles from mainDocument
    // this only matters when redrawRectangles is called between calls to update()
    QVector<GeoDataFolder*> folderList = rectangleFolder->folderList();
    foreach( GeoDataFolder *reliabilityRectangleFolder, folderList )
    {
        reliabilityRectangleFolder->clear();
        rectangleFolder->remove(rectangleFolder->childPosition(reliabilityRectangleFolder));
    }

    if (ui->checkEllipseAutoscale->isChecked())
    {
        currentEllipseFractionOfDiagonal = DEFAULT_FRACTION_OF_DIAGONAL;
    }

    if (ui->radioDisplayAdjusted->isChecked())
    {
        // find the max axis length of all ellipses and rectangles
        double maxAxisLengthMeters = getMaxAxisLengthMeters();
        double maxAxisLengthDegrees = calculateEllipseScaling(maxAxisLengthMeters);

        // add ellipses
        if(maxAxisLengthMeters > RECTANGLE_MIN_SIZE_THRESHOLD)
        {
            foreach (const FinalAdjustedPosition& point, finalPointMap)
            {
                QModelIndex pointIndex = pointIndexMap.value(QString::fromStdString(point.position));

                if (pointIndex.isValid() && guiModel->isActive(pointIndex))
                {
                    LSARecord* lsaRecord = guiModel->getLSARecord(pointIndex);
                    LSAType    lsaType = lsaRecord->getRecType();
                    bool pointIsFixed = true;
                    bool typeIsVisible = isTypeVisible(lsaType);
                    if (lsaType == LSAType::POSG)
                    {
                        LSAPosG* lsaPoint = static_cast<LSAPosG*>(lsaRecord);
                        pointIsFixed = lsaPoint->fixedState == LSAFixedState::FIXED;
                    }
                    else if (lsaType == LSAType::POSC)
                    {
                        LSAPosC* lsaPoint = static_cast<LSAPosC*>(lsaRecord);
                        pointIsFixed = lsaPoint->fixedState == LSAFixedState::FIXED;
                    }
                    else if (lsaType == LSAType::MEAN || lsaType == LSAType::ENUO)
                    {
                        pointIsFixed = false;
                    }

                    double scaledDegreesPerMeter = 0;
                    if (ui->checkDisplayRR->isChecked() && typeIsVisible)
                    {
                        scaledDegreesPerMeter = maxAxisLengthDegrees / maxAxisLengthMeters; // Used to convert rectangle sizes in meters to map plotting units of degrees
                    }

                    if (!pointIsFixed)
                    {
                           std::string rectangleName = lsaRecord->getLabel();
                           addPlacemark_Rectangle(rectangleName, pointIndex, getCoordinatesFromLabel(point.position), point.maj2Drr,
                                                  point.min2Drr, point.vertrr, point.azrr, scaledDegreesPerMeter);
                    }
                }
            }
        }
      }
    ui->map->model()->treeModel()->addDocument(&mainDocument);

    return true;
}

void Mapping::handleEllipses_Clicked()
{
   redrawEllipses();
   redrawRectangles();
   return;
}
void Mapping::handleRR_Clicked()
{
   redrawEllipses();
   redrawRectangles();
   return;
}

GeoDataLineString* Mapping::createAngle(const GeoDataCoordinates &from, const GeoDataCoordinates &at, const GeoDataCoordinates &to)
{
    const qreal scale = lonScaleFactor(at.latitude(), GeoDataCoordinates::Radian);
    const qreal inverseScale = 1.0 / scale;

    GeoDataLineString* angle = new GeoDataLineString();

    qreal fromLon = from.longitude(GeoDataCoordinates::Degree);
    qreal fromLat = from.latitude(GeoDataCoordinates::Degree);

    qreal atLon = at.longitude(GeoDataCoordinates::Degree);
    qreal atLat = at.latitude(GeoDataCoordinates::Degree);

    qreal toLon = to.longitude(GeoDataCoordinates::Degree);
    qreal toLat = to.latitude(GeoDataCoordinates::Degree);

    qreal distBetween_FromAt = distance(fromLon, fromLat, atLon, atLat);
    qreal distBetween_AtTo = distance(toLon, toLat, atLon, atLat);
    qreal angleRadius = ANGLE_SCALE * (qMin(distBetween_FromAt, distBetween_AtTo) * 2.0/3.0 + qMax(distBetween_FromAt,distBetween_AtTo) / 3.0);

    qreal angle_FromAt = qAtan2(fromLat - atLat, (fromLon - atLon) * inverseScale);
    qreal angle_AtTo   = qAtan2(toLat - atLat, (toLon - atLon) * inverseScale);

    qreal initialAngle = angle_FromAt;
    qreal finalAngle = angle_AtTo;

    /// the finalAngle must be smaller than the initialAngle
    /// to ensure that the angle is always plotted clockwise
    if (initialAngle < finalAngle) finalAngle -= 2 * Pi;

    qreal lon;
    qreal lat;

    qreal lon_secondToLast;
    qreal lat_secondToLast;

    for (int i = 0; i <= ANGLE_PRECISION; ++i)
    {
        qreal theta = initialAngle + (finalAngle - initialAngle) * ((qreal)(i) / (qreal)(ANGLE_PRECISION));
        lon = atLon + scale * angleRadius * qCos(theta);
        lat = atLat + angleRadius * qSin(theta);

        /// are we on the second to last iteration?
        if (ANGLE_PRECISION - i == 1)
        {
            /// if so, save the second to last coordinates
            lon_secondToLast = lon;
            lat_secondToLast = lat;
        }

        *angle << GeoDataCoordinates(lon, lat, DEFAULT_HEIGHT, GeoDataCoordinates::Degree);
    }

    // the angle to the horizontal (latiutide lines) of the second-to-last point from the last point
    qreal angleLastSecondToLast = qAtan2(lat_secondToLast - lat, (lon_secondToLast - lon) * inverseScale);

    // the point of one small line
    *angle << GeoDataCoordinates(lon + scale * ARROW_SCALE_FACTOR * angleRadius * qCos(angleLastSecondToLast + ARROW_ANGLE),
                                 lat + ARROW_SCALE_FACTOR * angleRadius * qSin(angleLastSecondToLast + ARROW_ANGLE),
                                 DEFAULT_HEIGHT, GeoDataCoordinates::Degree);
    // the end of the main line
    *angle << GeoDataCoordinates(lon, lat, DEFAULT_HEIGHT, GeoDataCoordinates::Degree);

    // the point of the other small line
    *angle << GeoDataCoordinates(lon + scale * ARROW_SCALE_FACTOR * angleRadius * qCos(angleLastSecondToLast - ARROW_ANGLE),
                                 lat + ARROW_SCALE_FACTOR * angleRadius * qSin(angleLastSecondToLast - ARROW_ANGLE),
                                 DEFAULT_HEIGHT, GeoDataCoordinates::Degree);

    return angle;
}

GeoDataLinearRing* Mapping::createEllipseFromCovariance(const GeoDataCoordinates &centerPosition, qreal Cnn, qreal Cne, qreal Cnu, qreal Cee,
                                                        qreal Ceu, qreal Cuu, qreal scaleDegreesPerMeter) {
    /// finds the lengths of the axis and angle of rotation of the ellipse,
    /// then call createEllipse to create the ellipse

    //construct NEU covariance matrix
    gnsstk::Matrix<double> Cov(3,3);
    Cov(0,0) = Cnn;
    Cov(0,1) = Cne;
    Cov(0,2) = Cnu;
    Cov(1,0) = Cne;
    Cov(1,1) = Cee;
    Cov(1,2) = Ceu;
    Cov(2,0) = Cnu;
    Cov(2,1) = Ceu;
    Cov(2,2) = Cuu;

    double maj3, min3, maj2, min2, vert, azm;
    getEllipse(Cov,maj3,min3,maj2,min2,vert,azm);

    //apply slider scale and user preference scale, and convert semi-axis to axis
    maj2 *= 2.0*scaleDegreesPerMeter;
    min2 *= 2.0*scaleDegreesPerMeter;

    //convert from azimuth angle to counter-clockwise angle from the x- (East-) axis
    double theta = 90.0 - azm;
    if(theta < 0.0)
        theta += 360.0;
    theta *= ::DEG_TO_RAD;

    return createEllipse(centerPosition, maj2, min2, theta);
}

GeoDataLineString* Mapping::createEllipseHeightFromCovariance(const GeoDataCoordinates &centerPosition, qreal Cuu, qreal scale) {
    //get vertical error in meters
    double vert = Cuu;
    vert = std::sqrt(vert);

    //meters to latitude
    double degLat = vert*ellipseBarConfidenceFactor1D*scale;
    double radLat = (degLat * M_PI)/180;

    GeoDataCoordinates toPosition = GeoDataCoordinates(centerPosition.longitude(), centerPosition.latitude() + radLat, GeoDataCoordinates::Radian);

    // create an object to hold the geometry of the line
    GeoDataLineString *lineString = new GeoDataLineString();

    // add points to the line
    lineString->append(centerPosition);
    lineString->append(toPosition);

    return lineString;
}

GeoDataLineString* Mapping::createRectangleHeight(const GeoDataCoordinates &centerPosition, qreal vertRR, qreal scale) {
    //meters to latitude
    double vertrr = vertRR;

    double degLat = vertrr*scale;
    double radLat = (degLat * M_PI)/180;

    /// So it points down
    GeoDataCoordinates toPosition = GeoDataCoordinates(centerPosition.longitude(), centerPosition.latitude() - radLat, GeoDataCoordinates::Radian);

    // create an object to hold the geometry of the line
    GeoDataLineString *lineString = new GeoDataLineString();

    // add points to the line
    lineString->append(centerPosition);
    lineString->append(toPosition);

    return lineString;
}

GeoDataLinearRing* Mapping::createEllipse(const GeoDataCoordinates &centerPosition, qreal width, qreal height, qreal angle)
{

    const qreal scale = lonScaleFactor(centerPosition.latitude(), GeoDataCoordinates::Radian);
//    const qreal inverseScale = 1.0 / scale;

    qreal centerLon = centerPosition.longitude(GeoDataCoordinates::Degree);
    qreal centerLat = centerPosition.latitude(GeoDataCoordinates::Degree);
    qreal altitude = centerPosition.altitude();

    GeoDataLinearRing* ellipse = new GeoDataLinearRing();

    qreal cosTheta = qCos(angle);
    qreal sinTheta = qSin(angle);

    /// This approximation of an ellipse uses the parameterized form of a rotated ellipse
    ///
    /// lon(t') = x = h + a * cos(t') * cos(theta) - b * sin(t') * sin(theta)
    /// lat(t') = y = k + a * cos(t') * sin(theta) + b * sin(t') * cos(theta)
    ///
    /// where
    /// a is half of the length of the x-axis (1/2 * width)
    /// b is half of the length of the y-axis (1/2 * height)
    /// (h,k) is the center of the ellipse [ (centerLon, centerLat) = (h,k) ]
    /// theta is the angle that the original x-axis of the ellipse has been rotated
    /// from the x-axis (longitude) in the final projection

    qreal a = 0.5 * width;
    qreal b = 0.5 * height;

    for ( int i = 0; i <= ELLIPSE_PRECISON; ++i )
    {
        qreal t = 2 * Pi * (qreal)(i) / (qreal)(ELLIPSE_PRECISON);
        qreal lon = centerLon + scale * (a * qCos(t) * cosTheta - b * qSin(t) * sinTheta);
        qreal lat = centerLat + a * qCos(t) * sinTheta + b * qSin(t) * cosTheta;
        *ellipse << GeoDataCoordinates( lon, lat, altitude, GeoDataCoordinates::Degree );
    }

    return ellipse;
}

GeoDataLinearRing* Mapping::createRectangle(const GeoDataCoordinates &centerPosition, qreal maj2Drr, qreal min2Drr, qreal angle, qreal scaleDegreesPerMeter)
{
    //apply slider scale and user preference scale, and convert semi-axis to axis
    maj2Drr *= 2.0*scaleDegreesPerMeter;
    min2Drr *= 2.0*scaleDegreesPerMeter;

    //convert from azimuth angle(degrees) to counter-clockwise angle from the x- (East-) axis
    double theta = 90.0 - angle;
    if(theta < 0.0)
    {
        theta += 360.0;
    }
    theta *= ::DEG_TO_RAD;

    const qreal scale = lonScaleFactor(centerPosition.latitude(), GeoDataCoordinates::Radian);
//    const qreal inverseScale = 1.0 / scale;

    qreal centerLon = centerPosition.longitude(GeoDataCoordinates::Degree);
    qreal centerLat = centerPosition.latitude(GeoDataCoordinates::Degree);
    qreal altitude = centerPosition.altitude();

    GeoDataLinearRing* rectangle = new GeoDataLinearRing();

    qreal cosTheta = qCos(theta);
    qreal sinTheta = qSin(theta);


    qreal a = 0.5*maj2Drr;
    qreal b = 0.5*min2Drr;

    for ( int i = 0; i <= ELLIPSE_PRECISON; ++i )
    {
        qreal t = 2 * Pi * (qreal)(i) / (qreal)(ELLIPSE_PRECISON);
        qreal lon = centerLon + scale * (a * copysign(1.0,qCos(t)) * cosTheta - b * copysign(1.0,qSin(t)) * sinTheta);
        qreal lat = centerLat + a * copysign(1.0,qCos(t)) * sinTheta + b * copysign(1.0,qSin(t)) * cosTheta;
        *rectangle << GeoDataCoordinates( lon, lat, altitude, GeoDataCoordinates::Degree );
    }

    return rectangle;
}

qreal Mapping::majorAxisLength(qreal covNN, qreal covNE, qreal covEE)
{
    qreal a = covNN;
    qreal b = covNE;
    qreal d = covEE;

    qreal eigenvalue1 = 0.5 * (a + d + qSqrt((a - d) * (a - d) + 4 * b * b)); // this is the larger eigenvalue
    qreal eigenvalue2 = 0.5 * (a + d - qSqrt((a - d) * (a - d) + 4 * b * b)); // this is the smaller eigenvalue

    return qMax(2 * qSqrt(eigenvalue1),2 * qSqrt(eigenvalue2));
}

void Mapping::addDerived(QModelIndex index)
{
    if (!index.isValid() ) return;

    GeoDataCoordinates coordinates = getCoordinatesFromIndex(index);
    IconType fixedStatus = getPointFixedStatus(index);
    std::string label = guiModel->getLSARecord(index)->getLabel();

    //Fix for bug #1170
    if(pointIndexMap.contains(QString::fromStdString(label)) || points.contains(index))
        return;

    if( guiModel->isActive(index) )
    {
        pointIndexMap.insert(QString::fromStdString(label), QPersistentModelIndex(index));
    }

    if(ui->radioDisplayAdjusted->isChecked() && adjustedCoordinateExists(label))//Fix to Bug 990
    {
        addPlacemark_Point(QString::fromStdString(label), coordinates, index, fixedStatus, getHighlightStatus(index));
    }

    return;
}

void Mapping::addPOSG(QModelIndex index)
{
    if (!index.isValid() ) return;

    GeoDataCoordinates coordinates = getCoordinatesFromIndex(index);
    IconType fixedStatus = getPointFixedStatus(index);
    std::string label = guiModel->getLSARecord(index)->getLabel();

    if( guiModel->isActive(index) )
    {
        pointIndexMap.insert(QString::fromStdString(label), QPersistentModelIndex(index));
    }

    if(ui->radioDisplayInitial->isChecked() || (ui->radioDisplayAdjusted->isChecked() && adjustedCoordinateExists(label)))//Fix to Bug 990
    {
        addPlacemark_Point(QString::fromStdString(label), coordinates, index, fixedStatus, getHighlightStatus(index));
    }

    return;
}

void Mapping::addPOSC(QModelIndex index)
{
    if (!index.isValid() ) return;

    GeoDataCoordinates coordinates = getCoordinatesFromIndex(index);
    IconType fixedStatus = getPointFixedStatus(index);
    std::string label = guiModel->getLSARecord(index)->getLabel();
    if( guiModel->isActive(index) )
    {
        pointIndexMap.insert(QString::fromStdString(label), QPersistentModelIndex(index));
    }
    if(ui->radioDisplayInitial->isChecked() || (ui->radioDisplayAdjusted->isChecked() && adjustedCoordinateExists(label)))//Fix to Bug 990
    {
        addPlacemark_Point(QString::fromStdString(label), coordinates, index, fixedStatus, getHighlightStatus(index));
    }

    return;
}

void Mapping::addDGRP(QModelIndex index)
{
    if (!index.isValid() ) return;

    std::string label = guiModel->getLSARecord(index)->getLabel();
    dirGroupIndexMap.insert(QString::fromStdString(label), QPersistentModelIndex(index));

    return;
}

void Mapping::addDIST(QModelIndex index)
{
    if ( !index.isValid() ) return;

    LSARecord* record = guiModel->getLSARecord(index);
    Q_ASSERT(record->getRecType() == LSAType::DIST);
    LSADist* dist = static_cast<LSADist*>(record);

    // don't add the measurement if one of the points doesn't have a coordinate yet
    if ( !coordinateExists(dist->From) || !coordinateExists(dist->To))
    {
        return;
    }

    GeoDataCoordinates fromCoordinate = getCoordinatesFromLabel(dist->From);
    GeoDataCoordinates toCoordinate   = getCoordinatesFromLabel(dist->To);
    addPlacemark_Line(fromCoordinate, toCoordinate, index, getHighlightStatus(index));

    return;
}

void Mapping::addDXYZ(QModelIndex index)
{
    if ( !index.isValid() ) return;

    LSARecord* record = guiModel->getLSARecord(index);
    Q_ASSERT(record->getRecType() == LSAType::DXYZ);
    LSADelta* dist = static_cast<LSADelta*>(record);

    // don't add the measurement if one of the points doesn't have a coordinate yet
    if ( !coordinateExists(dist->From) || !coordinateExists(dist->To))
        return;

    GeoDataCoordinates fromCoordinate = getCoordinatesFromLabel(dist->From);
    GeoDataCoordinates toCoordinate = getCoordinatesFromLabel(dist->To);
    addPlacemark_Line(fromCoordinate, toCoordinate, index, getHighlightStatus(index));

    return;
}

void Mapping::addHANG(QModelIndex index)
{
    if ( !index.isValid() ) return;

    LSARecord* record = guiModel->getLSARecord(index);
    Q_ASSERT(record->getRecType() == LSAType::HANG);
    LSAHAngle* angle = static_cast<LSAHAngle*>(record);

    // don't add the measurement if one of the points doesn't have a coordinate yet
    if ( !coordinateExists(angle->From) || !coordinateExists(angle->At) || !coordinateExists(angle->To))
        return;


    GeoDataCoordinates fromCoordinate = getCoordinatesFromLabel(angle->From);
    GeoDataCoordinates atCoordinate = getCoordinatesFromLabel(angle->At);
    GeoDataCoordinates toCoordinate = getCoordinatesFromLabel(angle->To);
    addPlacemark_Angle(fromCoordinate, atCoordinate, toCoordinate, index, getHighlightStatus(index));

    return;
}

void Mapping::addAZIM(QModelIndex index)
{
    if ( !index.isValid() ) return;

    LSARecord* record = guiModel->getLSARecord(index);
    Q_ASSERT(record->getRecType() == LSAType::AZIM);
    LSAAzimuth* azimuth = static_cast<LSAAzimuth*>(record);

    // don't add the measurement if one of the points doesn't have a coordinate yet
    if ( !coordinateExists(azimuth->From) || !coordinateExists(azimuth->To))
    {
        return;
    }

    GeoDataCoordinates fromCoordinate = getCoordinatesFromLabel(azimuth->From);
    GeoDataCoordinates toCoordinate = getCoordinatesFromLabel(azimuth->To);

    double orientationFactor = 1.0;
    if(azimuth->fromDir == std::string("S"))
        orientationFactor = -1.0;

    GeoDataCoordinates northPoint(fromCoordinate.longitude(GeoDataCoordinates::Degree),
                                  fromCoordinate.latitude(GeoDataCoordinates::Degree) +
                                                          AZIM_LENGTH_FACTOR * orientationFactor *
                                  distance(fromCoordinate.longitude(GeoDataCoordinates::Degree),
                                           fromCoordinate.latitude(GeoDataCoordinates::Degree),
                                           toCoordinate.longitude(GeoDataCoordinates::Degree),
                                           toCoordinate.latitude(GeoDataCoordinates::Degree)),
                                  DEFAULT_HEIGHT, GeoDataCoordinates::Degree);

    addPlacemark_Angle(northPoint, fromCoordinate, toCoordinate, index, getHighlightStatus(index));

    return;
}

void Mapping::addZANG(QModelIndex index)
{
    if ( !index.isValid() ) return;

    LSARecord* record = guiModel->getLSARecord(index);
    Q_ASSERT(record->getRecType() == LSAType::ZANG);
    LSAAzimuth* angle = static_cast<LSAAzimuth*>(record);

    // don't add the measurement if one of the points doesn't have a coordinate yet
    if ( !coordinateExists(angle->From) || !coordinateExists(angle->To))
    {
        return;
    }

    GeoDataCoordinates fromCoordinate = getCoordinatesFromLabel(angle->From);
    GeoDataCoordinates toCoordinate = getCoordinatesFromLabel(angle->To);
    addPlacemark_Line(fromCoordinate, toCoordinate, index, getHighlightStatus(index));

    return;
}

void Mapping::addHDIF(QModelIndex index)
{
    if ( !index.isValid() ) return;

    LSARecord* record = guiModel->getLSARecord(index);
    Q_ASSERT(record->getRecType() == LSAType::HDIF);
    LSAHeightDiff* diff = static_cast<LSAHeightDiff*>(record);

    // don't add the measurement if one of the points doesn't have a coordinate yet
    if ( !coordinateExists(diff->From) || !coordinateExists(diff->To))
    {
        return;
    }

    GeoDataCoordinates fromCoordinate = getCoordinatesFromLabel(diff->From);
    GeoDataCoordinates toCoordinate = getCoordinatesFromLabel(diff->To);
    addPlacemark_Line(fromCoordinate, toCoordinate, index, getHighlightStatus(index));

    return;
}

void Mapping::addHDIR(QModelIndex index)
{
    if ( !index.isValid() ) return;

    LSARecord* record = guiModel->getLSARecord(index);
    Q_ASSERT(record->getRecType() == LSAType::HDIR);
    LSAHDir* hdir = static_cast<LSAHDir*>(record);

    // don't add the measurement if one of the points doesn't have a coordinate yet or
    // if the dirGroupLabel string is not in dirGroupIndexMap
    if (dirGroupIndexMap.find(QString::fromStdString(hdir->dirGroupLabel)) != dirGroupIndexMap.end())
    {
        if (coordinateExists(hdir->toLabel))
        {
            QModelIndex dgrpIndex = dirGroupIndexMap.value(QString::fromStdString(hdir->dirGroupLabel));
            if (dgrpIndex.isValid())
            {
                LSADirGroup* dgrp = static_cast<LSADirGroup*>(guiModel->getLSARecord(dgrpIndex));
                if (coordinateExists(dgrp->fromLabel))
                {
                    GeoDataCoordinates toCoordinate = getCoordinatesFromLabel(hdir->toLabel);
                    GeoDataCoordinates fromCoordinate = getCoordinatesFromLabel(dgrp->fromLabel);
                    addPlacemark_Line(toCoordinate, fromCoordinate, index, getHighlightStatus(index));
                }
            }
        }
        else
        {
            // we found a hdir without a dgrp
        }
    }

    return;
}

void Mapping::addVANG(QModelIndex index)
{
    if ( !index.isValid() ) return;

    LSARecord* record = guiModel->getLSARecord(index);
    Q_ASSERT(record->getRecType() == LSAType::VANG);
    LSAVAngle* angle = static_cast<LSAVAngle*>(record);

    // don't add the measurement if one of the points doesn't have a coordinate yet
    if ( !coordinateExists(angle->From) || !coordinateExists(angle->To))
    {
        return;
    }

    GeoDataCoordinates fromCoordinate = getCoordinatesFromLabel(angle->From);
    GeoDataCoordinates toCoordinate = getCoordinatesFromLabel(angle->To);
    addPlacemark_Line(fromCoordinate, toCoordinate, index, getHighlightStatus(index));


    return;
}

IconType Mapping::getPointFixedStatus(QModelIndex index)
{
    if ( !index.isValid() )
        return POINT_FLOATING;

    LSARecord* point = guiModel->getLSARecord(index);
    IconType status = UNKNOWN;
    int numFixed = 0;

    if (point->getRecType() == LSAType::POSG)
    {
        //cast LSARecord type to LSAPosG
        LSAPosG* posg = static_cast<LSAPosG*>(point);

        if (posg->isAutogenerated)
            status = POINT_APRIORI;
        else if(posg->fixedState == LSAFixedState::FIXED)
            status = POINT_FIXED_ALL;
        else if(posg->fixedState == LSAFixedState::CONSTRAINED)
            status = POINT_CONSTRAIN;
        else if(posg->fixedState == LSAFixedState::FLOATING)
            status = POINT_FLOATING;

        if (posg->isEastFixed) ++numFixed;
        if (posg->isNorthFixed) ++numFixed;
        if (posg->isUpFixed) ++numFixed;
    }
    else if (point->getRecType() == LSAType::POSC)
    {
        //cast LSARecord type to LSAPosG
        LSAPosC* posc = static_cast<LSAPosC*>(point);

        if (posc->isAutogenerated)
            status = POINT_APRIORI;
        else if(posc->fixedState == LSAFixedState::FIXED)
            status = POINT_FIXED_ALL;
        else if(posc->fixedState == LSAFixedState::CONSTRAINED)
            status = POINT_CONSTRAIN;
        else if(posc->fixedState == LSAFixedState::FLOATING)
            status = POINT_FLOATING;

        if (posc->isEastFixed)
            numFixed++;
        if (posc->isNorthFixed)
            numFixed++;
        if (posc->isUpFixed)
            numFixed++;
    }
    else if (point->getRecType() == LSAType::MEAN || point->getRecType() == LSAType::ENUO)
    {
        //derived point
        status = POINT_FLOATING;
    }


    if(POINT_FLOATING == status){
        switch (numFixed) {
        case 0:
            status = POINT_FLOATING;
            break;
        case 1:
            status = POINT_FIXED_1;
            break;
        case 2:
            status = POINT_FIXED_2;
            break;
        case 3:
            status = POINT_FIXED_ALL;
            break;
        }
    }

     // fix to bug #1065
     else if(POINT_CONSTRAIN == status){
         switch (numFixed) {
         case 0:
             status = POINT_CONSTRAIN;
             break;
         case 1:
             status = POINT_FIXED_1;
             break;
         case 2:
             status = POINT_FIXED_2;
             break;
         case 3:
             status = POINT_FIXED_ALL;
             break;
         }
     }

    return status;

}

HighlightStatus Mapping::getHighlightStatus(QModelIndex index)
{
    if (index.isValid())
    {
        HighlightStatus status;
        if ( guiModel->indexIsSelected(index) && guiModel->isActive(index) && isTypeVisible(guiModel->getLSARecord(index)->getRecType()))
        {
            status = HIGHLIGHTED;
        } else
        {
            status = DEFAULT;
        }

        return status;
    }
    return DEFAULT;
}

// Clears all points and measurements from the map
void Mapping::clear()
{
    ui->map->model()->treeModel()->removeDocument(&mainDocument);

    // This call clears the document's internal memory structure and
    // calls delete on all associated placemarks.  Placemarks automatically
    // delete their geometries in their destructors.  Thus no additionall memory
    // management of all of the dynamically allocated memory from addPoint(), addLine(),
    // addAngle(), addEllipseFromCovariance() is necessary.
    mainDocument.clear();
    points.clear();
    lines.clear();
    lineHighlights.clear();
    angles.clear();
    angleHighlights.clear();
    angleReferenceLines.clear();
    angleReferenceLineHighlights.clear();
    ellipses.clear();
    ellipseVerticalBars.clear();
    rectangles.clear();
    rectangleVerticalBars.clear();

    pointIndexMap.clear();
    dirGroupIndexMap.clear();
    finalPointMap.clear();

    placemarkToIndexMap.clear();
    indexToPlacemarkMap.clear();

    iconStyleFixed1.remoteIconLoader()->disconnect();
    iconStyleFixed2.remoteIconLoader()->disconnect();
    iconStyleFixedAll.remoteIconLoader()->disconnect();
    iconStyleFloating.remoteIconLoader()->disconnect();
    iconStyleDerived.remoteIconLoader()->disconnect();
    iconStyleAPriori.remoteIconLoader()->disconnect();

    iconStyleFixed1Highlighted.remoteIconLoader()->disconnect();
    iconStyleFixed2Highlighted.remoteIconLoader()->disconnect();
    iconStyleFixedAllHighlighted.remoteIconLoader()->disconnect();
    iconStyleFloatingHighlighted.remoteIconLoader()->disconnect();
    iconStyleDerivedHighlighted.remoteIconLoader()->disconnect();
    iconStyleAPrioriHighlighted.remoteIconLoader()->disconnect();


//    ellipseHighlights.clear();
}

void Mapping::unHighlightAll()
{
    // points
    foreach(GeoDataPlacemark *point, points)
    {
        QSharedPointer<const Marble::GeoDataStyle> origStyle = point->style();
        point->setVisualCategory(GeoDataPlacemark::GeoDataVisualCategory(LSAVisualCategory::Default));
        point->setZoomLevel(1);

        if (point->style() == styleFixed1Highlighted)   point->setStyle(styleFixed1);
        if (point->style() == styleFixed2Highlighted)   point->setStyle(styleFixed2);
        if (point->style() == styleFixedAllHighlighted) point->setStyle(styleFixedAll);
        if (point->style() == styleFloatingHighlighted) point->setStyle(styleFloating);
        if (point->style() == styleDerivedHighlighted)  point->setStyle(styleDerived);
        if (point->style() == styleConstrainHighlighted)point->setStyle(styleConstrain);
        if (point->style() == styleAPrioriHighlighted)  point->setStyle(styleAPriori);

        if(origStyle != point->style())
        {
            emit point->style()->iconStyle().remoteIconLoader()->iconReady();
        }

    }



    // lines
    foreach(GeoDataPlacemark* lineHighlight, lineHighlights)
    {
        lineHighlight->setVisible(false);
    }
    for(auto& pair : lines.toStdMap())
    {
        LSARecord* rec = guiModel->getLSARecord(pair.first);
        if(rec)
        {
            auto type = rec->getRecType();
            pair.second->setVisualCategory(getVisualCategoryForType(type, HighlightStatus::DEFAULT));
        }
    }

    // angles
    foreach(GeoDataPlacemark* angleHighlight, angleHighlights)
    {
        angleHighlight->setVisible(false);
    }
    for(auto& pair : angles.toStdMap())
    {
        LSARecord* rec = guiModel->getLSARecord(pair.first);
        if(rec)
        {
            auto type = rec->getRecType();
            pair.second->setVisualCategory(getVisualCategoryForType(type, HighlightStatus::DEFAULT));
        }
    }

    // angle reference lines
    foreach(GeoDataPlacemark* angleReferenceLineHighlight, angleReferenceLineHighlights)
    {
        angleReferenceLineHighlight->setVisible(false);
    }
    for(auto& pair : angleReferenceLines.toStdMap())
    {
        LSARecord* rec = guiModel->getLSARecord(pair.first);
        if(rec)
        {
            auto type = rec->getRecType();
            pair.second->setVisualCategory(getVisualCategoryForType(type, HighlightStatus::DEFAULT));
        }
    }
}

void Mapping::unHighlightPlacemarksForIndex(QModelIndex index)
{
    // points
    foreach(GeoDataPlacemark *point, points.values(index) )
    {
        QSharedPointer<const Marble::GeoDataStyle> origStyle = point->style();
        point->setVisualCategory(GeoDataPlacemark::GeoDataVisualCategory(LSAVisualCategory::Default));
        point->setZoomLevel(1);

        if (point->style() == styleFixed1Highlighted)   point->setStyle(styleFixed1);
        if (point->style() == styleFixed2Highlighted)   point->setStyle(styleFixed2);
        if (point->style() == styleFixedAllHighlighted) point->setStyle(styleFixedAll);
        if (point->style() == styleFloatingHighlighted) point->setStyle(styleFloating);
        if (point->style() == styleDerivedHighlighted)  point->setStyle(styleDerived);
        if (point->style() == styleAPrioriHighlighted)  point->setStyle(styleAPriori);

        if(origStyle != point->style())
        {
            emit point->style()->iconStyle().remoteIconLoader()->iconReady();
        }

    }

    // lines
    foreach(GeoDataPlacemark* lineHighlight, lineHighlights.values(index) )
    {
        lineHighlight->setVisible(false);
    }

    // angles
    foreach(GeoDataPlacemark* angleHighlight, angleHighlights.values(index) )
    {
        angleHighlight->setVisible(false);
    }

    // angle reference lines
    foreach(GeoDataPlacemark* angleReferenceLineHighlight, angleReferenceLineHighlights.values(index) )
    {
        angleReferenceLineHighlight->setVisible(false);
    }

    return;
}

void Mapping::highlight(QModelIndex index, GeoDataPlacemark* placemark)
{
    LSARecord *lsaRecord = guiModel->getLSARecord(index);
    if (lsaRecord == NULL)
    {
        return;
    }

    bool typeIsVisible = isTypeVisible(guiModel->getLSARecord(index)->getRecType());
    bool recordIsActive = guiModel->isActive(index);

    if(typeIsVisible && recordIsActive)
    {
        if (points.contains(index))
        {
            const GeoDataIconStyle origStyle = placemark->style()->iconStyle();
            placemark->setVisualCategory(GeoDataPlacemark::GeoDataVisualCategory(LSAVisualCategory::Selected));
            placemark->setZoomLevel(0);

            if (placemark->style() == styleFixed1)   placemark->setStyle(styleFixed1Highlighted);
            if (placemark->style() == styleFixed2)   placemark->setStyle(styleFixed2Highlighted);
            if (placemark->style() == styleFixedAll) placemark->setStyle(styleFixedAllHighlighted);
            if (placemark->style() == styleFloating) placemark->setStyle(styleFloatingHighlighted);
            if (placemark->style() == styleDerived)  placemark->setStyle(styleDerivedHighlighted);
            if (placemark->style() == styleConstrain)placemark->setStyle(styleConstrainHighlighted);
            if (placemark->style() == styleAPriori)  placemark->setStyle(styleAPrioriHighlighted);

            if(origStyle != placemark->style()->iconStyle())
            {
                emit placemark->style()->iconStyle().remoteIconLoader()->iconReady();
            }
        }
        else if (lines.contains(index))
        {
            foreach(GeoDataPlacemark* lineHighlight, lineHighlights.values(index))
            {
                lineHighlight->setVisible(true);
            }
            placemark->setVisualCategory(GeoDataPlacemark::GeoDataVisualCategory(LSAVisualCategory::Selected));
        }
        else if (angles.contains(index) || angleReferenceLines.contains(index))
        {
            foreach(GeoDataPlacemark* angleHighlight, angleHighlights.values(index))
            {
                angleHighlight->setVisible(true);
            }

            foreach(GeoDataPlacemark* angleReferenceLineHighlight, angleReferenceLineHighlights.values(index))
            {
                angleReferenceLineHighlight->setVisible(true);
            }
            placemark->setVisualCategory(GeoDataPlacemark::GeoDataVisualCategory(LSAVisualCategory::Selected));
        }
    }
}

/// returns a qvector of all placemarks "near" to the given position,
/// where "near" is defined according to the map's current zoom level
///
/// If givePointsPriority (defaults to false) is set to true and a point is "near" the given
/// position, the output vector will consist solely of points.
/// If there are no points "near" the given point, the output is the same
/// regardless of this flag.
QMap<QModelIndex, GeoDataPlacemark*> Mapping::findAllNear(const QPointF &centerPoint, bool givePointsPriority)
{
    // the following statement designates the radius of the icon given a specific zoom level
    // this model was found from an exponential regression on zoom data
    //
//    qreal iconClickRadius = 0.5 * 1271.1520679195    // terms from regression
//                            * qExp( -0.0049781448   // term from regression
//                                    * ui->map->zoom() )     // the zool level of the map
//                            * 0.0174532925  // conversion factor from degrees (the unit the model was produced in) to marbles default unit: radians
//                            * 1.5;  // user experience factor: about how much around the icon the user can be expected to click

    const qreal iconClickRadius = 12.0;
    const qreal lineClickRadius = 5.0;

    QMap<QModelIndex, GeoDataPlacemark*> nearPlacemarks;
    GeoDataCoordinates::Unit unit = GeoDataCoordinates::Degree;

    bool pointFound = false;
    // select the point
    foreach(GeoDataPlacemark *point, points)
    {
        QPointF pointPos = pointFromCoordinate( point->coordinate() );

        if (iconClickRadius > distance(pointPos, centerPoint))
        {
            pointFound = true;

            nearPlacemarks.insert(placemarkToIndexMap.value(point), point);
        }
    }

    // only search through the measurements if the click didn't correspond to a point
    if (!(pointFound && givePointsPriority))
    {
        foreach(GeoDataPlacemark *line, lines)
        {
            if (isNearEnough_Line(centerPoint, line, lineClickRadius))
            {
                nearPlacemarks.insert(placemarkToIndexMap.value(line), line);
            }
        }
    }

    if (!(pointFound && givePointsPriority))
    {
        foreach(GeoDataPlacemark *angle, angles)
        {
            if (isNearEnough_Line(centerPoint, angle, lineClickRadius))
            {
                nearPlacemarks.insert(placemarkToIndexMap.value(angle), angle);
            }
        }
    }
    if (!(pointFound && givePointsPriority))
    {
        foreach(GeoDataPlacemark *angleReferenceLine, angleReferenceLines)
        {
            if (isNearEnough_Line(centerPoint, angleReferenceLine, lineClickRadius))
            {
                nearPlacemarks.insert(placemarkToIndexMap.value(angleReferenceLine), angleReferenceLine);
            }
        }
    }

    return nearPlacemarks;
}

bool Mapping::isNearEnough_Line(const QPointF &centerPoint, GeoDataPlacemark* placemark, qreal radius)
{
    bool isNearEnough = false;

    GeoDataLineString line = static_cast<GeoDataLineString>(*(placemark->geometry()));

    // loop through all sequential pairs of points
    for (int i = 0; i < line.size() - 1; ++i) {

        /// the distance from a point (m,n) to the line Ax + By + C = 0
        /// is given by:
        ///
        /// d = abs(Am+Bn+C)/(A^2 + B^2)^(1/2)
        ///
        /// if the line is given by two points (x_1, y_1) and (x_2, y_2)
        ///
        /// A = (y_2 - y_1)/(x_2 - x_1)
        /// B = -1
        /// C = -A * x_1 + y_1

        QPointF point1 = pointFromCoordinate(line[i]);
        QPointF point2 = pointFromCoordinate(line[i+1]);

        qreal x1 = point1.x();
        qreal x2 = point2.x();
        qreal y1 = point1.y();
        qreal y2 = point2.y();

        qreal distance;

        // don't find the slope of a vertical line
        if (x1 != x2) {

            qreal A = (y2 - y1) / (x2 - x1);
            qreal B = -1;
            qreal C = -A * x1 + y1;

            distance = (A * centerPoint.x() + B * centerPoint.y() + C) / qSqrt(A * A + B * B);
            if (distance < 0) distance = -1 * distance;
        } else {
            distance = 0.0;  // for a vertical line the bound check will be enough
        }

        // for the comparison x1 must be less than or
        // equal to x2
        if (x1 > x2) {
            qreal temp;
            temp = x1;
            x1 = x2;
            x2 = temp;
        }

        x1 = (x1 - radius); // the final factor is arbitrary and should be determined from user experience
        x2 = (x2 + radius);


        if (y1 > y2) {
            qreal temp;
            temp = y1;
            y1 = y2;
            y2 = temp;
        }

        y1 = (y1 - radius);
        y2 = (y2 + radius);

        // returns true if the point is within the box given by the end points of the line segment (with tolerance) and the
        // point is within radius distance of the line segment
        if (x1 < centerPoint.x() &&
                x2 > centerPoint.x() &&
                y1 < centerPoint.y() &&
                y2 > centerPoint.y() &&
                radius > distance) {
            isNearEnough = true;
            break;  // we know the point is near enough to the line string, we don't need to check other segments of the same line string

        }
    }
    return isNearEnough;
}

qreal Mapping::distance(qreal x1, qreal y1, qreal x2, qreal y2)
{
    return qSqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

qreal Mapping::distance(const QPointF &point1, const QPointF &point2)
{
    qreal x1 = point1.x();
    qreal y1 = point1.y();
    qreal x2 = point2.x();
    qreal y2 = point2.y();

    return qSqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void Mapping::appendPlacemark(GeoDataPlacemark* placemark, GeoDataDocument* doc) {
    /// The program will crash (seg fault) if the mainDocument is not removed before being
    /// changed
//    ui->map->model()->treeModel()->removeDocument(mainDocument);

    /// add the placemark to the document
    doc->append(placemark);

    /// add the document back to the map
//    ui->map->model()->treeModel()->addDocument( mainDocument );
}

GeoDataCoordinates Mapping::getCoordinateFromPOSG(LSAPosG* point) {
    qreal lon;
    qreal lat;

    if (point->useDecimalDegrees)
    {
        lat = point->latDecDeg;
        lon = point->lonDecDeg;
    } else {
        lat = DMSToDeg(point->latDMSIsNeg, point->latDeg, point->latMin, point->latSec);
        lon = DMSToDeg(point->lonDMSIsNeg, point->lonDeg, point->lonMin, point->lonSec);
    }

    lon = lonFromLonDir(lon, point->lonDir);
    lat = latFromLatDir(lat, point->latDir);

    GeoDataCoordinates::normalizeLonLat(lon,lat, GeoDataCoordinates::Degree);
    return GeoDataCoordinates(lon, lat, DEFAULT_HEIGHT, GeoDataCoordinates::Degree);
}

GeoDataCoordinates Mapping::getCoordinateFromPOSC(LSAPosC* point)
{
    gnsstk::Position position(point->x,point->y,point->z);
    position = position.asGeodetic();

    double lon = position.getLongitude();
    double lat = position.getGeodeticLatitude();
    GeoDataCoordinates::normalizeLonLat(lon,lat, GeoDataCoordinates::Degree);
    GeoDataCoordinates coordinate(lon, lat, DEFAULT_HEIGHT, GeoDataCoordinates::Degree);
    return coordinate;
}

bool Mapping::isPointDisplayedInMap(std::string posLabel)
{
    QString label = QString::fromStdString(posLabel);
    if(pointIndexMap.contains(label))
    {
        QPersistentModelIndex index = pointIndexMap[label];
        if(indexToPlacemarkMap.contains(index))
        {
            QList<GeoDataPlacemark*> placemarks = indexToPlacemarkMap.values(index);
            foreach(GeoDataPlacemark* placemark, placemarks)
            {
                QModelIndex index = QModelIndex(placemarkToIndexMap[placemark]);
                int a=0;
            }

            if(placemarks.size()>0)
                return true;
        }
    }
    return false;
}

bool Mapping::initialCoordinateExists(std::string str)
{
    QString label = QString::fromStdString(str);
    return pointIndexMap.find(label) != pointIndexMap.end();
}

bool Mapping::adjustedCoordinateExists(std::string str)
{
    QString label = QString::fromStdString(str);
    return finalPointMap.find(label) != finalPointMap.end();
}

bool Mapping::coordinateExists(std::string label)
{
    bool initalCoordFound = ui->radioDisplayInitial->isChecked() && initialCoordinateExists(label);
    bool adjustCoordFound = ui->radioDisplayAdjusted->isChecked() && adjustedCoordinateExists(label);

    return initalCoordFound || adjustCoordFound;
}

/// warning, the output of this function is garbage if label does not coorespond to a point in either point map
GeoDataCoordinates Mapping::getCoordinatesFromLabel(std::string str)
{   
    GeoDataCoordinates pointCoordinates;

    if (ui->radioDisplayAdjusted->isChecked())
    {
        pointCoordinates = getFinalCoordinateFromLabel(str);
    }
    else
    {
        pointCoordinates = getInitialCoordinateFromLabel(str);
    }

    return pointCoordinates;
}

GeoDataCoordinates Mapping::getCoordinatesFromIndex(QModelIndex index)
{
    LSARecord  *lsaRecord = guiModel->getLSARecord(index);
    LSAType     lsaType   = lsaRecord->getRecType();
    std::string label     = lsaRecord->getLabel();

    if((lsaType != LSAType::POSC) &&
       (lsaType != LSAType::POSG) &&
       (lsaType != LSAType::MEAN) &&
       (lsaType != LSAType::ENUO))
    {
        return GeoDataCoordinates();
    }

    // Use final coordinate if selected and available
    if ( ui->radioDisplayAdjusted->isChecked() && adjustedCoordinateExists(label) )
    {
        if (adjustedCoordinateExists(label))
        {
            return getFinalCoordinateFromLabel(label);
        }
    }

    // Otherwise use the initial coordinate
    if (lsaType == LSAType::POSG)
    {
        LSAPosG *lsaPosG = static_cast<LSAPosG*>(lsaRecord);
        return getCoordinateFromPOSG(lsaPosG);
    }
    else if (lsaType == LSAType::POSC)
    {
        LSAPosC *lsaPosC = static_cast<LSAPosC*>(lsaRecord);
        return getCoordinateFromPOSC(lsaPosC);
    }
    else if ((lsaType == LSAType::MEAN) || (lsaType == LSAType::ENUO))
    {
        return GeoDataCoordinates();//no initial points for derived points
    }

    return GeoDataCoordinates(); // should never get to this point
}

GeoDataCoordinates Mapping::getInitialCoordinateFromLabel(std::string str)
{
    // Verify an initial coordinate exists
    if (!initialCoordinateExists(str))
        return GeoDataCoordinates();

    // Verify the point label is present in the pointIndexMap
    QString label = QString::fromStdString(str);
    QModelIndex index = pointIndexMap[label];
    if (!index.isValid())
        return GeoDataCoordinates();

    // Get the coordinates
    LSARecord* point = guiModel->getLSARecord(index);
    GeoDataCoordinates pointCoordinates;
    if (point->getRecType() == LSAType::POSG)
    {
        pointCoordinates = getCoordinateFromPOSG(static_cast<LSAPosG *>(point));
    }
    else if (point->getRecType() == LSAType::POSC)
    {
        pointCoordinates = getCoordinateFromPOSC(static_cast<LSAPosC *>(point));
    }

    return pointCoordinates;
}

GeoDataCoordinates Mapping::getFinalCoordinateFromLabel(std::string str)
{
    // Verify an initial coordinate exists
    if (!adjustedCoordinateExists(str))
        return GeoDataCoordinates();

    // Get the coordinates
    QString label = QString::fromStdString(str);
    GeoDataCoordinates pointCoordinates;
    if (adjustedCoordinateExists(str))
    {
        FinalAdjustedPosition point = finalPointMap[label];
        qreal lat = latFromLatDir(point.latDecDeg, point.latDir);
        qreal lon = lonFromLonDir(point.lonDecDeg, point.lonDir);
        pointCoordinates = GeoDataCoordinates(lon, lat, DEFAULT_HEIGHT, GeoDataCoordinates::Degree);
    }

    return pointCoordinates;
}


qreal Mapping::latFromLatDir(qreal lat, std::string latDir) {
    if (latDir[0] == 'S') {
        lat *= -1.0;
    }
    return GeoDataCoordinates::normalizeLat(lat, GeoDataCoordinates::Degree);
}

qreal Mapping::lonFromLonDir(qreal lon, std::string lonDir) {
    if (lonDir[0] == 'W') {
        lon *= -1.0;
    }
    return GeoDataCoordinates::normalizeLon(lon, GeoDataCoordinates::Degree);
}

QPointF Mapping::pointFromCoordinate(const GeoDataCoordinates &coor) {
    qreal x, y;
    ui->map->screenCoordinates(coor.longitude(GeoDataCoordinates::Degree), coor.latitude(GeoDataCoordinates::Degree), x, y);
    QPointF point(x,y);
    return point;
}

GeoDataCoordinates Mapping::coordinateFromPoint(const QPointF &point) {
    qreal lon, lat;
    ui->map->geoCoordinates(qRound(point.x()), qRound(point.y()), lon, lat);
    GeoDataCoordinates coor(lon, lat, DEFAULT_HEIGHT, GeoDataCoordinates::Degree);
    return coor;
}

qreal Mapping::lonScaleFactor(qreal lat, GeoDataCoordinates::Unit unit) {
    if (unit == GeoDataCoordinates::Degree) {
        lat *= DEG2RAD;
    }
    if (qCos(lat) == 0) {
        return 1;
    } else {
        return qAbs(1.0 / qCos(lat));
    }
}

void Mapping::showBackgroundMap()
{
    ui->map->setMapThemeId(MAP_THEME_OPENSTREET);
    configureMap();
}

void Mapping::hideBackgroundMap()
{
    ui->map->setMapThemeId(MAP_THEME_BLANK);
    configureMap();
}

void Mapping::configureMap() {
    /// Option to remove the overview map
    ui->map->setShowOverviewMap(false);

    /// Option to remove the map crosshairs
    ui->map->setShowCrosshairs(false);

    /// Option to remove the compass
    ui->map->setShowCompass(false);
}

qreal Mapping::getDegLatPerPixel()
{
    QRect windowRectangle = QRect(0,0,ui->map->width(),ui->map->height());
    GeoDataLatLonAltBox viewBox = ui->map->viewport()->latLonAltBox(windowRectangle);
    qreal north = viewBox.north(GeoDataCoordinates::Degree);
    qreal south = viewBox.south(GeoDataCoordinates::Degree);
    return qAbs((north - south) / ui->map->height());
}

qreal Mapping::getMapDiagonalPixels()
{
    qreal height = ui->map->height();
    qreal width = ui->map->width();
    return qSqrt(height * height + width * width);
}

void Mapping::on_checkEllipseAutoscale_clicked() {
    if (ui->checkEllipseAutoscale->isChecked()) {
        ui->doubleEllipseScale->setEnabled(false);
        ui->sliderEllipseScale->setEnabled(false);
    } else {
        ui->doubleEllipseScale->setEnabled(true);
        ui->sliderEllipseScale->setEnabled(true);
    }
    redrawEllipses();
    redrawRectangles();
}

qreal Mapping::getMaxAxisLengthMeters()
{
   /// find the max axis length of all ellipses
   qreal maxAxisLength = 0.0, maxRectangleLength = 0.0, maxLength = 0.0;
   qreal maxAxisHeight = 0.0, maxRectangleHeight = 0.0;

   foreach (const FinalAdjustedPosition& point, finalPointMap)
   {
      qreal axisLength, rectangleLength;

      /// Check largest ellipse axis size
      if(ui->checkDisplayEllipses->isChecked())
      {
         axisLength = majorAxisLength(point.covNN, point.covNE, point.covEE) * ellipseConfidenceFactor2D;//scale by APV and condfidence region factor

         if (axisLength > maxAxisLength)
         {
            maxAxisLength = axisLength;
         }
      }

      /// Check largest rectangle axis size
      if(ui->checkDisplayRR->isChecked())
      {
         rectangleLength = 2*sqrt(point.maj2Drr*point.maj2Drr + point.min2Drr*point.min2Drr);

         if(rectangleLength > maxRectangleLength)
         {
            maxRectangleLength = rectangleLength;
         }
      }
   }

   if(maxAxisLength > maxRectangleLength)
   {
      maxLength = maxAxisLength;
   }
   else
   {
      maxLength = maxRectangleLength;
   }


   // Fix to bug 1054. If the 2D axis is too small in 1D corrections (usually ~=0),
   // the vertical bars aren't visible.
   if(maxLength < 0.001)
   {
      foreach (const FinalAdjustedPosition& point, finalPointMap)
      {
         qreal axisHeight, rectangleHeight;

         /// Check largest ellipse axis size
         if(ui->checkDisplayEllipses->isChecked())
         {
            axisHeight = sqrt(point.covUU) * ellipseConfidenceFactor2D;//scale by APV and condfidence region factor

            if (axisHeight > maxAxisHeight)
            {
               maxAxisHeight = axisHeight;
            }
         }

         /// Check largest rectangle axis size
         if(ui->checkDisplayRR->isChecked())
         {
            rectangleHeight = point.vertrr;

            if(rectangleHeight > maxRectangleHeight)
            {
               maxRectangleHeight = rectangleHeight;
            }
         }
      }

      // Set the max axis length as the largest height value
      if(maxAxisHeight > maxRectangleHeight)
      {
         maxLength = maxAxisHeight;
      }
      else
      {
         maxLength = maxRectangleHeight;
      }
   }

   return maxLength;
}

void Mapping::handleSliderMoved(int action)
{
    Q_UNUSED(action) // disable compiler warnings
    handleSliderReleased();
}

void Mapping::handleSliderReleased()
{
    if (!ui->sliderEllipseScale->isEnabled()) return;

    if (ui->sliderEllipseScale->maximum() == 0)
    {
        currentEllipseFractionOfDiagonal = 0;
    } else
    {
        currentEllipseFractionOfDiagonal = ((qreal) ui->sliderEllipseScale->value() ) / (ui->sliderEllipseScale->maximum());
    }
//        qDebug() << "slider" << currentFractionOfDiagonal;
    if (currentEllipseFractionOfDiagonal == 0)
    {
        ui->doubleEllipseScale->setValue(ui->doubleEllipseScale->maximum());
    }
    else
    {
        qreal metersPerPixel = getMaxAxisLengthMeters() / (getMapDiagonalPixels() * currentEllipseFractionOfDiagonal);
        ui->doubleEllipseScale->setValue(metersPerPixel * SCALE_BAR_WIDTH_PIXELS);
    }


    redrawEllipses();
    redrawRectangles();
}

void Mapping::on_doubleEllipseScale_valueChanged(double arg1)
{
    Q_UNUSED(arg1) // disable compiler warnings
    if (ui->doubleEllipseScale->isEnabled() && ui->doubleEllipseScale->hasFocus())
    {
        currentEllipseFractionOfDiagonal = getMaxAxisLengthMeters() /  (ui->doubleEllipseScale->value() / SCALE_BAR_WIDTH_PIXELS * getMapDiagonalPixels());
        ui->sliderEllipseScale->setValue(currentEllipseFractionOfDiagonal * (ui->sliderEllipseScale->maximum()));
        redrawEllipses();
        redrawRectangles();

    }
}

void Mapping::updateStyles()
{
    // DIST
    DISTLineStyle.setColor(getColorDIST());
    DISTLineStyleHighlighted.setColor(getColorHighlighting());
    DISTLineStyle.setWidth(getLineWidthDIST());
    DISTLineStyle.setPenStyle((Qt::PenStyle)getPenStyleDIST());
    DISTLineStyleHighlighted.setWidth(getHighlightWidth(getLineWidthDIST()));
    DISTStyle->setLineStyle(DISTLineStyle);
    DISTStyleHighlighted->setLineStyle(DISTLineStyleHighlighted);

    // DXYZ
    DXYZLineStyle.setColor(getColorDXYZ());
    DXYZLineStyleHighlighted.setColor(getColorHighlighting());
    DXYZLineStyle.setWidth(getLineWidthDXYZ());
    DXYZLineStyle.setPenStyle((Qt::PenStyle)getPenStyleDXYZ());
    DXYZLineStyleHighlighted.setWidth(getHighlightWidth(getLineWidthDXYZ()));
    DXYZStyle->setLineStyle(DXYZLineStyle);
    DXYZStyleHighlighted->setLineStyle(DXYZLineStyleHighlighted);

    // ZANG
    ZANGLineStyle.setColor(getColorZANG());
    ZANGLineStyleHighlighted.setColor(getColorHighlighting());
    ZANGLineStyle.setWidth(getLineWidthZANG());
    ZANGLineStyle.setPenStyle((Qt::PenStyle)getPenStyleZANG());
    ZANGLineStyleHighlighted.setWidth(getHighlightWidth(getLineWidthZANG()));
    ZANGStyle->setLineStyle(ZANGLineStyle);
    ZANGStyleHighlighted->setLineStyle(ZANGLineStyleHighlighted);

    // HDIR
    HDIRLineStyle.setColor(getColorHDIR());
    HDIRLineStyleHighlighted.setColor(getColorHighlighting());
    HDIRLineStyle.setWidth(getLineWidthHDIR());
    HDIRLineStyle.setPenStyle((Qt::PenStyle)getPenStyleHDIR());
    HDIRLineStyleHighlighted.setWidth(getHighlightWidth(getLineWidthHDIR()));
    HDIRStyle->setLineStyle(HDIRLineStyle);
    HDIRStyleHighlighted->setLineStyle(HDIRLineStyleHighlighted);

    // HDIF
    HDIFLineStyle.setColor(getColorHDIF());
    HDIFLineStyleHighlighted.setColor(getColorHighlighting());
    HDIFLineStyle.setWidth(getLineWidthHDIF());
    HDIFLineStyle.setPenStyle((Qt::PenStyle)getPenStyleHDIF());
    HDIFLineStyleHighlighted.setWidth(getHighlightWidth(getLineWidthHDIF()));
    HDIFStyle->setLineStyle(HDIFLineStyle);
    HDIFStyleHighlighted->setLineStyle(HDIFLineStyleHighlighted);

    // VANG
    VANGLineStyle.setColor(getColorVANG());
    VANGLineStyleHighlighted.setColor(getColorHighlighting());
    VANGLineStyle.setWidth(getLineWidthVANG());
    VANGLineStyle.setPenStyle((Qt::PenStyle)getPenStyleVANG());
    VANGLineStyleHighlighted.setWidth(getHighlightWidth(getLineWidthVANG()));
    VANGStyle->setLineStyle(VANGLineStyle);
    VANGStyleHighlighted->setLineStyle(VANGLineStyleHighlighted);

    // AZIM
    AZIMLineStyle.setColor(getColorAZIM());
    AZIMLineStyleHighlighted.setColor(getColorHighlighting());
    AZIMLineStyle.setWidth(getLineWidthAZIM());
    AZIMLineStyle.setPenStyle((Qt::PenStyle)getPenStyleAZIM());
    AZIMLineStyleHighlighted.setWidth(getHighlightWidth(getLineWidthAZIM()));
    AZIMStyle->setLineStyle(AZIMLineStyle);
    AZIMStyleHighlighted->setLineStyle(AZIMLineStyleHighlighted);

    // AZIM reference
    AZIMReferenceLineLineStyle.setColor(getColorAZIMReferenceLines());
    AZIMReferenceLineLineStyleHighlighted.setColor(getColorHighlighting());
    AZIMReferenceLineLineStyle.setWidth(getLineWidthAZIMReferenceLines());
    AZIMReferenceLineLineStyle.setPenStyle((Qt::PenStyle)getPenStyleAZIMReferenceLines());
    AZIMReferenceLineLineStyleHighlighted.setWidth(getHighlightWidth(getLineWidthAZIMReferenceLines()));
    AZIMReferenceLineStyle->setLineStyle(AZIMReferenceLineLineStyle);
    AZIMReferenceLineStyleHighlighted->setLineStyle(AZIMReferenceLineLineStyleHighlighted);

    // HANG
    HANGLineStyle.setColor(getColorHANG());
    HANGLineStyleHighlighted.setColor(getColorHighlighting());
    HANGLineStyle.setWidth(getLineWidthHANG());
    HANGLineStyle.setPenStyle((Qt::PenStyle)getPenStyleHANG());
    HANGLineStyleHighlighted.setWidth(getHighlightWidth(getLineWidthHANG()));
    HANGStyle->setLineStyle(HANGLineStyle);
    HANGStyleHighlighted->setLineStyle(HANGLineStyleHighlighted);

    // HANG reference
    HANGReferenceLineLineStyle.setColor(getColorHANGReferenceLines());
    HANGReferenceLineLineStyleHighlighted.setColor(getColorHighlighting());
    HANGReferenceLineLineStyle.setWidth(getLineWidthHANGReferenceLines());
    HANGReferenceLineLineStyle.setPenStyle((Qt::PenStyle)getPenStyleHANGReferenceLines());
    HANGReferenceLineLineStyleHighlighted.setWidth(getHighlightWidth(getLineWidthHANGReferenceLines()));
    HANGReferenceLineStyle->setLineStyle(HANGReferenceLineLineStyle);
    HANGReferenceLineStyleHighlighted->setLineStyle(HANGReferenceLineLineStyleHighlighted);

    // Error ellipse
    ellipseLineStyle.setColor(getColorEllipse());
    ellipseLineStyle.setWidth(getLineWidthEllipse());
    ellipseLineStyle.setPenStyle((Qt::PenStyle)getPenStyleEllipse());
    ellipsePolyStyle.setFill(false);
    ellipseStyle->setLineStyle(ellipseLineStyle);
    ellipseStyle->setPolyStyle(ellipsePolyStyle);

    // Reliability Rectangle
    rectangleLineStyle.setColor(getColorRectangle());
    rectangleLineStyle.setWidth(getLineWidthRectangle());
    rectangleLineStyle.setPenStyle((Qt::PenStyle)getPenStyleRectangle());
    rectanglePolyStyle.setFill(false);
    rectangleStyle->setLineStyle(rectangleLineStyle);
    rectangleStyle->setPolyStyle(rectanglePolyStyle);

    return;
}

void Mapping::handleMapOptionsTriggered()
{
    pushOptionsDialog();
}

void Mapping::handleEnableDisableClicked()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, lsa::salsaOrgName, std::getenv("salsaAppName"));
    // Toggle the enabled state and button text
    if ( mapIsEnabled )
    {
        mapIsEnabled = false;
        settings.setValue( lsa::QSETTINGS_MAP_ENABLED, mapIsEnabled );
        ui->btnEnableDisable->setText("Enable");
    }
    else
    {
        // Get the current map zoom extents
        qreal lon = ui->map->centerLongitude();
        qreal lat = ui->map->centerLatitude();
        qreal zoom = ui->map->zoom();

        initialize();
        mapIsEnabled = true;
        settings.setValue( lsa::QSETTINGS_MAP_ENABLED, mapIsEnabled );
        ui->btnEnableDisable->setText("Disable");

        // Set the map back to its previous zoom settings
        ui->map->setCenterLatitude(lat);
        ui->map->setCenterLongitude(lon);
        ui->map->setZoom(zoom);
    }

    // Enable/disable all of the map controls
    ui->map->setEnabled(mapIsEnabled);
    ui->frameCheckboxes->setEnabled(mapIsEnabled);
    ui->frameEllipseScaling->setEnabled(mapIsEnabled);
    ui->frameInputOutputRadios->setEnabled(mapIsEnabled);
    ui->checkDisplayMapLayer->setEnabled(mapIsEnabled);
    ui->checkDisplayRR->setEnabled(mapIsEnabled);


    if (mapIsEnabled)
    {
        redraw();
    }

    return;
}

void Mapping::pushOptionsDialog() {
    OptionsDialog dialog(this, this);
    if (dialog.exec() == QDialog::Accepted) { // if the user closes the dialog by pressing the "OK" button
        // write dialog values to mapping
        dialog.pushToMapping();
        updateStyles();
        redraw();
    }
}

void Mapping::useDefaultStyle()
{
    //AZIM
    setColorAZIM(lsa::DEFAULT_COLOR_AZIM);
    setLineWidthAZIM(lsa::DEFAULT_LINEWIDTH_AZIM);
    setPenStyleAZIM(lsa::DEFAULT_PENSTYLE_AZIM);

    // AZIM Reference Lines
    setColorAZIMReferenceLines(lsa::DEFAULT_COLOR_AZIM_REF);
    setLineWidthAZIMReferenceLines(lsa::DEFAULT_LINEWIDTH_AZIM_REF);
    setPenStyleAZIMReferenceLines(lsa::DEFAULT_PENSTYLE_AZIM_REF);

    // HANG
    setColorHANG(lsa::DEFAULT_COLOR_HANG);
    setLineWidthHANG(lsa::DEFAULT_LINEWIDTH_HANG);
    setPenStyleHANG(lsa::DEFAULT_PENSTYLE_HANG);

    // HANG Reference Lines
    setColorHANGReferenceLines(lsa::DEFAULT_COLOR_HANG_REF);
    setLineWidthHANGReferenceLines(lsa::DEFAULT_LINEWIDTH_HANG_REF);
    setPenStyleHANGReferenceLines(lsa::DEFAULT_PENSTYLE_HANG_REF);

    // ZANG
    setColorZANG(lsa::DEFAULT_COLOR_ZANG);
    setLineWidthZANG(lsa::DEFAULT_LINEWIDTH_ZANG);
    setPenStyleZANG(lsa::DEFAULT_PENSTYLE_ZANG);

    // DIST
    setColorDIST(lsa::DEFAULT_COLOR_DIST);
    setLineWidthDIST(lsa::DEFAULT_LINEWIDTH_DIST);
    setPenStyleDIST(lsa::DEFAULT_PENSTYLE_DIST);

    // DXYZ
    setColorDXYZ(lsa::DEFAULT_COLOR_DXYZ);
    setLineWidthDXYZ(lsa::DEFAULT_LINEWIDTH_DXYZ);
    setPenStyleDXYZ(lsa::DEFAULT_PENSTYLE_DXYZ);

    // HDIR
    setColorHDIR(lsa::DEFAULT_COLOR_HDIR);
    setLineWidthHDIR(lsa::DEFAULT_LINEWIDTH_HDIR);
    setPenStyleHDIR(lsa::DEFAULT_PENSTYLE_HDIR);

    // HDIF
    setColorHDIF(lsa::DEFAULT_COLOR_HDIF);
    setLineWidthHDIF(lsa::DEFAULT_LINEWIDTH_HDIF);
    setPenStyleHDIF(lsa::DEFAULT_PENSTYLE_HDIF);

    // VANG
    setColorVANG(lsa::DEFAULT_COLOR_VANG);
    setLineWidthVANG(lsa::DEFAULT_LINEWIDTH_VANG);
    setPenStyleVANG(lsa::DEFAULT_PENSTYLE_VANG);

    // Ellipse
    setColorEllipse(lsa::DEFAULT_COLOR_ELLIPSE);
    setLineWidthEllipse(lsa::DEFAULT_LINEWIDTH_ELLIPSE);
    setPenStyleEllipse(lsa::DEFAULT_PENSTYLE_ELLIPSE);

    // Rectangle
    setColorRectangle(lsa::DEFAULT_COLOR_RECTANGLE);
    setLineWidthRectangle(lsa::DEFAULT_LINEWIDTH_RECTANGLE);
    setPenStyleRectangle(lsa::DEFAULT_PENSTYLE_RECTANGLE);

    // Highlighting
    setColorHighlighting(lsa::DEFAULT_COLOR_HIGHLIGHT);
}

void Mapping::setScaleControlsVisible(bool visible)
{
    ui->frameEllipseScaling->setVisible(visible);
}

void Mapping::copyOutputPositionsFromGuiModel()
{
    // create a map of point labels to output positions
    int numOutputPositions = guiModel->outputPositions.size();
    for (int i = 0; i < numOutputPositions; ++i)
    {
        FinalAdjustedPosition finalPosition = guiModel->outputPositions.at(i);
        finalPointMap.insert(QString::fromStdString(finalPosition.position), finalPosition);
    }
}

void Mapping::closeEvent (QCloseEvent *event)
{
    emit closing();
    event->accept();
}
