/*
 *    Copyright 2012, 2013, 2014 Thomas Schöps
 *    Copyright 2012, 2013, 2014 Kai Pastor
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


#include "global.h"

#include <mapper_config.h>

#include "core/crs_template.h"
#include "core/georeferencing.h"
#include "file_format_registry.h"
#include "file_format_native.h"
#include "file_format_ocad8.h"
#include "file_format_xml.h"
#include "fileformats/ocd_file_format.h"

void registerProjectionTemplates()
{
	// TODO: These objects are never deleted.
	
	/*
	 * CRSTemplate(
	 *     id,
	 *     name,
	 *     coordinates name,
	 *     specification string)
	 * 
	 * The id must be unique and different from "Local".
	 */
	
	// UTM
	CRSTemplate* temp = new CRSTemplate(
		"UTM",
		Georeferencing::tr("UTM", "UTM coordinate reference system"),
		Georeferencing::tr("UTM coordinates"),
		"+proj=utm +datum=WGS84 +zone=%1");
	temp->addParam(new CRSTemplate::UTMZoneParam(Georeferencing::tr("UTM Zone (number north/south, e.g. \"32 N\", \"24 S\")")));
	CRSTemplate::registerCRSTemplate(temp);
	
	// Gauss-Krueger
	temp = new CRSTemplate(
		"Gauss-Krueger, datum: Potsdam",
		Georeferencing::tr("Gauss-Krueger, datum: Potsdam", "Gauss-Krueger coordinate reference system"),
		Georeferencing::tr("Gauss-Krueger coordinates"),
		"+proj=tmerc +lat_0=0 +lon_0=%1 +k=1.000000 +x_0=%2 +y_0=0 +ellps=bessel +datum=potsdam +units=m +no_defs");
	temp->addParam((new CRSTemplate::IntRangeParam(Georeferencing::tr("Zone number (1 to 119)", "Zone number for Gauss-Krueger coordinates"), 1, 119))->clearOutputs()->addDerivedOutput(3, 0)->addDerivedOutput(1000000, 500000));
	CRSTemplate::registerCRSTemplate(temp);
}

void doStaticInitializations()
{
	// Register the supported file formats
	FileFormats.registerFormat(new XMLFileFormat());
	FileFormats.registerFormat(new OcdFileFormat());
#ifndef NO_NATIVE_FILE_FORMAT
	FileFormats.registerFormat(new NativeFileFormat()); // TODO: Remove before release 1.0
#endif
	
#if defined(Q_OS_ANDROID)
	// Register file finder function needed by Proj.4
	registerProjFileHelper();
#endif
	
	// Register projection templates
	registerProjectionTemplates();
}
