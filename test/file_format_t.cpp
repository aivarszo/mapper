/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
 *
 *    This file is part of OpenOrienteering.
 *
 *    OpenOrienteering is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenOrienteering is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "file_format_t.h"

#include "../src/core/map_color.h"
#include "../src/core/map_printer.h"
#include "../src/file_format_registry.h"
#include "../src/file_import_export.h"
#include "../src/core/georeferencing.h"
#include "../src/global.h"
#include "../src/core/map_grid.h"
#include "../src/mapper_resource.h"
#include "../src/object.h"
#include "../src/template.h"
#include "../src/undo_manager.h"



#ifdef QT_PRINTSUPPORT_LIB

namespace QTest
{
	template<>
	char* toString(const MapPrinterPageFormat& page_format)
	{
		QByteArray ba = "";
		ba += MapPrinter::paperSizeNames()[page_format.paper_size];
		ba += (page_format.orientation == MapPrinterPageFormat::Landscape) ? " landscape (" : " portrait (";
		ba += QByteArray::number(page_format.paper_dimensions.width(), 'f', 2) + "x";
		ba += QByteArray::number(page_format.paper_dimensions.height(), 'f', 2) + "), ";
		ba += QByteArray::number(page_format.page_rect.left(), 'f', 2) + ",";
		ba += QByteArray::number(page_format.page_rect.top(), 'f', 2) + "+";
		ba += QByteArray::number(page_format.page_rect.width(), 'f', 2) + "x";
		ba += QByteArray::number(page_format.page_rect.height(), 'f', 2) + ", overlap ";
		ba += QByteArray::number(page_format.h_overlap, 'f', 2) + ",";
		ba += QByteArray::number(page_format.v_overlap, 'f', 2);
		return qstrdup(ba.data());
	}
	
	template<>
	char* toString(const MapPrinterOptions& options)
	{
		QByteArray ba = "";
		ba += "1:" + QByteArray::number(options.scale) + ", ";
		ba += QByteArray::number(options.resolution) + " dpi, ";
		if (options.show_templates)
			ba += ", templates";
		if (options.show_grid)
			ba += ", grid";
		if (options.simulate_overprinting)
			ba += ", overprinting";
		return qstrdup(ba.data());
	}
}

#endif



void FileFormatTest::initTestCase()
{
	// Needed to load current developer preferences, if possible
	QCoreApplication::setOrganizationName("OpenOrienteering.org");
	QCoreApplication::setApplicationName("Mapper");
	
	doStaticInitializations();
	
	map_filenames 
	  << "COPY_OF_test_map.omap"
	  << "COPY_OF_spotcolor_overprint.xmap";
	  
	for (int i = 0; i < map_filenames.size(); ++i)
	{
		QString filename = MapperResource::locate(MapperResource::TEST_DATA, map_filenames[i]);
		QVERIFY2(!filename.isEmpty(), QString("Unable to locate %1").arg(map_filenames[i]).toLocal8Bit());
		map_filenames[i] = filename;
	}
}

void FileFormatTest::saveAndLoad_data()
{
	// Add all file formats which support import and export
	QTest::addColumn<QString>("format_id");
	QTest::addColumn<QString>("map_filename");
	
	Q_FOREACH(const FileFormat* format, FileFormats.formats())
	{
		if (format->supportsExport() && format->supportsImport())
		{
			Q_FOREACH(QString filename, map_filenames)
				QTest::newRow(format->id().toLocal8Bit()) << format->id() << filename;
		}
	}
}

void FileFormatTest::saveAndLoad()
{
	QFETCH(QString, format_id);
	QFETCH(QString, map_filename);
	
	// Find the file format and verify that it exists
	const FileFormat* format = FileFormats.findFormat(format_id);
	QVERIFY(format);
	
	// Load the test map
	Map* original = new Map();
	original->loadFrom(map_filename, NULL, NULL, false, false);
	QVERIFY(!original->hasUnsavedChanged());
	
	// Fix precision of grid rotation
	MapGrid grid = original->getGrid();
	grid.setAdditionalRotation(Georeferencing::roundDeclination(grid.getAdditionalRotation()));
	original->setGrid(grid);
	
	// Manipulate some data
	auto printer_config = original->printerConfig();
	printer_config.page_format.h_overlap += 2.0;
	printer_config.page_format.v_overlap += 4.0;
	original->setPrinterConfig(printer_config);
	
	// If the export is lossy, do one export / import cycle first to get rid of all information which cannot be exported into this format
	if (format->isExportLossy())
	{
		Map* new_map = saveAndLoadMap(original, format);
		if (!new_map)
			QFAIL("Exception while importing / exporting.");
		delete original;
		original = new_map;
	}
	
	// The test: save and load the map and compare the resulting maps
	Map* new_map = saveAndLoadMap(original, format);
	if (!new_map)
		QFAIL("Exception while importing / exporting.");
	
	QString error = "";
	bool equal = compareMaps(original, new_map, error);
	if (!equal)
		QFAIL(QString("Loaded map does not equal saved map, error: %1").arg(error).toLocal8Bit());
	
	comparePrinterConfig(new_map->printerConfig(), original->printerConfig());
	
	delete new_map;
	delete original;
}

Map* FileFormatTest::saveAndLoadMap(Map* input, const FileFormat* format)
{
	try {
		QBuffer buffer;
		buffer.open(QIODevice::ReadWrite);
		
		Exporter* exporter = format->createExporter(&buffer, input, NULL);
		if (!exporter)
			return NULL;
		exporter->doExport();
		delete exporter;
		
		buffer.seek(0);
		
		Map* out = new Map;
		Importer* importer = format->createImporter(&buffer, out, NULL);
		if (!importer)
			return NULL;
		importer->doImport(false);
		importer->finishImport();
		delete importer;
		
		buffer.close();
		
		return out;
	}
	catch (std::exception& e)
	{
		return NULL;
	}
}

bool FileFormatTest::compareMaps(const Map* a, const Map* b, QString& error)
{
	// TODO: This does not compare everything - yet ...
	
	// Miscellaneous
	
	if (a->getScaleDenominator() != b->getScaleDenominator())
	{
		error = "The map scales differ.";
		return false;
	}
	if (a->getMapNotes().compare(b->getMapNotes()) != 0)
	{
		error = "The map notes differ.";
		return false;
	}
	
	const Georeferencing& a_geo = a->getGeoreferencing();
	const Georeferencing& b_geo = b->getGeoreferencing();
	if (a_geo.getProjectedCRSId() == "Local coordinates")
		// Fix for old native OMAP test file
		const_cast<Georeferencing&>(a_geo).setProjectedCRS("Local", a_geo.getProjectedCRSSpec());
	
	if (a_geo.isLocal() != b_geo.isLocal() ||
		a_geo.getScaleDenominator() != b_geo.getScaleDenominator() ||
		a_geo.getDeclination() != b_geo.getDeclination() ||
		a_geo.getGrivation() != b_geo.getGrivation() ||
		a_geo.getMapRefPoint() != b_geo.getMapRefPoint() ||
		a_geo.getProjectedRefPoint() != b_geo.getProjectedRefPoint() ||
		a_geo.getProjectedCRSId() != b_geo.getProjectedCRSId() ||
		a_geo.getProjectedCRSName() != b_geo.getProjectedCRSName() ||
		a_geo.getProjectedCRSSpec() != b_geo.getProjectedCRSSpec() ||
		a_geo.getGeographicRefPoint().latitude() != b_geo.getGeographicRefPoint().latitude() ||
		a_geo.getGeographicRefPoint().longitude() != b_geo.getGeographicRefPoint().longitude())
	{
		error = "The georeferencing differs.";
		return false;
	}
	
	const MapGrid& a_grid = a->getGrid();
	const MapGrid& b_grid = b->getGrid();
	
	if (a_grid != b_grid || !(a_grid == b_grid))
	{
		error = "The map grid differs.";
		return false;
	}
	
	if (a->isAreaHatchingEnabled() != b->isAreaHatchingEnabled() ||
		a->isBaselineViewEnabled() != b->isBaselineViewEnabled())
	{
		error = "The view mode differs.";
		return false;
	}
	
	// Colors
	if (a->getNumColors() != b->getNumColors())
	{
		error = "The number of colors differs.";
		return false;
	}
	for (int i = 0; i < a->getNumColors(); ++i)
	{
		if (!a->getColor(i)->equals(*b->getColor(i), true))
		{
			error = QString("Color #%1 (%2) differs.").arg(i).arg(a->getColor(i)->getName());
			return false;
		}
	}
	
	// Symbols
	if (a->getNumSymbols() != b->getNumSymbols())
	{
		error = "The number of symbols differs.";
		return false;
	}
	for (int i = 0; i < a->getNumSymbols(); ++i)
	{
		const Symbol* a_symbol = a->getSymbol(i);
		if (!a_symbol->equals(b->getSymbol(i), Qt::CaseSensitive, true))
		{
			error = QString("Symbol #%1 (%2) differs.").arg(i).arg(a_symbol->getName());
			return false;
		}
	}
	
	// Parts and objects
	// TODO: Only a single part is compared here! Add comparison for parts ...
	if (a->getNumParts() != b->getNumParts())
	{
		error = "The number of parts differs.";
		return false;
	}
	if (a->getCurrentPartIndex() != b->getCurrentPartIndex())
	{
		error = "The current part differs.";
		return false;
	}
	for (int part = 0; part < a->getNumParts(); ++part)
	{
		MapPart* a_part = a->getPart(part);
		MapPart* b_part = b->getPart(part);
		if (a_part->getName().compare(b_part->getName(), Qt::CaseSensitive) != 0)
		{
			error = QString("The names of part #%1 differ (%2 <-> %3).").arg(part).arg(a_part->getName()).arg(b_part->getName());
			return false;
		}
		if (a_part->getNumObjects() != b_part->getNumObjects())
		{
			error = "The number of objects differs.";
			return false;
		}
		for (int i = 0; i < a_part->getNumObjects(); ++i)
		{
			if (!a_part->getObject(i)->equals(b_part->getObject(i), true))
			{
				error = QString("Object #%1 (with symbol %2) in part #%3 differs.").arg(i).arg(a_part->getObject(i)->getSymbol()->getName()).arg(part);
				return false;
			}
		}
	}
	
	// Object selection
	if (a->getNumSelectedObjects() != b->getNumSelectedObjects())
	{
		error = "The number of selected objects differs.";
		return false;
	}
	if ((a->getFirstSelectedObject() == NULL && b->getFirstSelectedObject() != NULL) || (a->getFirstSelectedObject() != NULL && b->getFirstSelectedObject() == NULL) ||
		(a->getFirstSelectedObject() != NULL && b->getFirstSelectedObject() != NULL &&
		 a->getCurrentPart()->findObjectIndex(a->getFirstSelectedObject()) != b->getCurrentPart()->findObjectIndex(b->getFirstSelectedObject())))
	{
		error = "The first selected object differs.";
		return false;
	}
	for (Map::ObjectSelection::const_iterator it = a->selectedObjectsBegin(), end = a->selectedObjectsEnd(); it != end; ++it)
	{
		if (!b->isObjectSelected(b->getCurrentPart()->getObject(a->getCurrentPart()->findObjectIndex(*it))))
		{
			error = "The selected objects differ.";
			return false;
		}
	}
	
	// Undo steps
	// TODO: Currently only the number of steps is compared here.
	if (a->undoManager().undoStepCount() != b->undoManager().undoStepCount() ||
		a->undoManager().redoStepCount() != b->undoManager().redoStepCount())
	{
		error = "The number of undo / redo steps differs.";
		return false;
	}
	if (a->undoManager().canUndo() &&
	    a->undoManager().nextUndoStep()->getType() != b->undoManager().nextUndoStep()->getType())
	{
		error = "The type of the first undo step is different.";
		return false;
	}
	if (a->undoManager().canRedo() &&
	    a->undoManager().nextRedoStep()->getType() != b->undoManager().nextRedoStep()->getType())
	{
		error = "The type of the first redo step is different.";
		return false;
	}
	
	// Templates
	if (a->getNumTemplates() != b->getNumTemplates())
	{
		error = "The number of templates differs.";
		return false;
	}
	if (a->getFirstFrontTemplate() != b->getFirstFrontTemplate())
	{
		error = "The division into front / back templates differs.";
		return false;
	}
	for (int i = 0; i < a->getNumTemplates(); ++i)
	{
		// TODO: only template filenames are compared
		if (a->getTemplate(i)->getTemplateFilename().compare(b->getTemplate(i)->getTemplateFilename()) != 0)
		{
			error = QString("Template filename #%1 differs.").arg(i);
			return false;
		}
	}
	
	return true;
}

 
void FileFormatTest::comparePrinterConfig(const MapPrinterConfig& copy, const MapPrinterConfig& orig)
{
	QCOMPARE(copy.center_print_area, orig.center_print_area);
	QCOMPARE(copy.options, orig.options);
	QCOMPARE(copy.page_format, orig.page_format);
	// Compare print area with reasonable precision, here: 0.1 mm
	QCOMPARE((copy.print_area.topLeft()*10.0).toPoint(), (orig.print_area.topLeft()*10.0).toPoint());
	QCOMPARE((copy.print_area.size()*10.0).toSize(), (orig.print_area.size()*10.0).toSize());
	QCOMPARE(copy.printer_name, orig.printer_name);
	QCOMPARE(copy.single_page_print_area, orig.single_page_print_area);
}


/*
 * We select a non-standard QPA because we don't need a real GUI window.
 * 
 * Normally, the "offscreen" plugin would be the correct one.
 * However, it bails out with a QFontDatabase error (cf. QTBUG-33674)
 */
auto qpa_selected = qputenv("QT_QPA_PLATFORM", "minimal");


QTEST_MAIN(FileFormatTest)
