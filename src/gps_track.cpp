/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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


#include "gps_track.h"

#include <QApplication>
#include <QFile>
#include <QHash>
#include <QInputDialog> // TODO: get rid of this
#include <QMessageBox>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <mapper_config.h> // TODO: Replace APP_NAME by runtime function to remove this dependency

#include "dxfparser.h"
#include "template_track.h"


namespace
{
	// Shared definition of standard geographic CRS.
	// TODO: Merge with Georeferencing.
	static const QString geographic_crs_spec = "+proj=latlong +datum=WGS84";
}


// There is some (mis?)use of TrackPoint's gps_coord LatLon
// as sort-of MapCoordF.
// This function serves both for explicit conversion and highlighting.
MapCoordF fakeMapCoordF(const LatLon &latlon)
{
	return MapCoordF(latlon.longitude(), latlon.latitude());
}

TrackPoint::TrackPoint(LatLon coord, QDateTime datetime, float elevation, int num_satellites, float hDOP)
{
	gps_coord = coord;
	is_curve_start = false;
	this->datetime = datetime;
	this->elevation = elevation;
	this->num_satellites = num_satellites;
	this->hDOP = hDOP;
}
void TrackPoint::save(QXmlStreamWriter* stream) const
{
	stream->writeAttribute("lat", QString::number(gps_coord.latitude(), 'f', 12));
	stream->writeAttribute("lon", QString::number(gps_coord.longitude(), 'f', 12));
	
	if (datetime.isValid())
		stream->writeTextElement("time", datetime.toString(Qt::ISODate) + 'Z');
	if (elevation > -9999)
		stream->writeTextElement("ele", QString::number(elevation, 'f', 3));
	if (num_satellites >= 0)
		stream->writeTextElement("sat", QString::number(num_satellites));
	if (hDOP >= 0)
		stream->writeTextElement("hdop", QString::number(hDOP, 'f', 3));
}



// ### Track ###

Track::Track() : track_crs(NULL)
{
	current_segment_finished = true;
}

Track::Track(const Georeferencing& map_georef) : track_crs(NULL), map_georef(map_georef)
{
	current_segment_finished = true;
}

Track::Track(const Track& other)
{
	waypoints = other.waypoints;
	waypoint_names = other.waypoint_names;
	
	segment_points = other.segment_points;
	segment_starts = other.segment_starts;
	segment_names  = other.segment_names;
	
	current_segment_finished = other.current_segment_finished;
	
	element_tags   = other.element_tags;
	
	map_georef = other.map_georef;
	
	if (other.track_crs != NULL)
	{
		track_crs = new Georeferencing(*other.track_crs);
	}
}

Track::~Track()
{
	delete track_crs;
}

Track& Track::operator=(const Track& rhs)
{
	clear();
	
	waypoints = rhs.waypoints;
	waypoint_names = rhs.waypoint_names;
	
	segment_points = rhs.segment_points;
	segment_starts = rhs.segment_starts;
	segment_names  = rhs.segment_names;
	
	current_segment_finished = rhs.current_segment_finished;
	
	element_tags   = rhs.element_tags;
	
	map_georef = rhs.map_georef;
	
	if (rhs.track_crs != NULL)
	{
		track_crs = new Georeferencing(*rhs.track_crs);
	}
	
	return *this;
}

void Track::clear()
{
	waypoints.clear();
	waypoint_names.clear();
	segment_points.clear();
	segment_starts.clear();
	segment_names.clear();
	current_segment_finished = true;
	element_tags.clear();
	delete track_crs;
	track_crs = NULL;
}

bool Track::loadFrom(const QString& path, bool project_points, QWidget* dialog_parent)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	clear();

	if (path.endsWith(".gpx", Qt::CaseInsensitive))
	{
		if (!loadFromGPX(&file, project_points, dialog_parent))
			return false;
	}
	else if (path.endsWith(".dxf", Qt::CaseInsensitive))
	{
		if (!loadFromDXF(&file, project_points, dialog_parent))
			return false;
	}
	else if (path.endsWith(".osm", Qt::CaseInsensitive))
	{
		if (!loadFromOSM(&file, project_points, dialog_parent))
			return false;
	}
	else
		return false;

	file.close();
	return true;
}
bool Track::saveTo(const QString& path) const
{
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;
	
	QXmlStreamWriter stream(&file);
	stream.writeStartDocument();
	stream.writeStartElement("gpx");
	stream.writeAttribute("version", "1.1");
	stream.writeAttribute("creator", APP_NAME);
	
	int size = getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		stream.writeStartElement("wpt");
		const TrackPoint& point = getWaypoint(i);
		point.save(&stream);
		stream.writeTextElement("name", waypoint_names[i]);
		stream.writeEndElement();
	}
	
	stream.writeStartElement("trk");
	for (int i = 0; i < getNumSegments(); ++i)
	{
		stream.writeStartElement("trkseg");
		size = getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			stream.writeStartElement("trkpt");
			const TrackPoint& point = getSegmentPoint(i, k);
			point.save(&stream);
			stream.writeEndElement();
		}
		stream.writeEndElement();
	}
	stream.writeEndElement();
	
	stream.writeEndElement();
	stream.writeEndDocument();
	
	file.close();
	return true;
}

void Track::appendTrackPoint(TrackPoint& point)
{
	point.map_coord = map_georef.toMapCoordF(point.gps_coord, NULL); // TODO: check for errors
	segment_points.push_back(point);
	
	if (current_segment_finished)
	{
		segment_starts.push_back(segment_points.size() - 1);
		current_segment_finished = false;
	}
}
void Track::finishCurrentSegment()
{
	current_segment_finished = true;
}

void Track::appendWaypoint(TrackPoint& point, const QString& name)
{
	point.map_coord = map_georef.toMapCoordF(point.gps_coord, NULL); // TODO: check for errors
	waypoints.push_back(point);
	waypoint_names.push_back(name);
}

void Track::changeMapGeoreferencing(const Georeferencing& new_map_georef)
{
	map_georef = new_map_georef;
	
	projectPoints();
}

void Track::setTrackCRS(Georeferencing* track_crs)
{
	delete this->track_crs;
	this->track_crs = track_crs;
	
	projectPoints();
}

int Track::getNumSegments() const
{
	return (int)segment_starts.size();
}

int Track::getSegmentPointCount(int segment_number) const
{
	Q_ASSERT(segment_number >= 0 && segment_number < (int)segment_starts.size());
	if (segment_number == (int)segment_starts.size() - 1)
		return segment_points.size() - segment_starts[segment_number];
	else
		return segment_starts[segment_number + 1] - segment_starts[segment_number];
}

const TrackPoint& Track::getSegmentPoint(int segment_number, int point_number) const
{
	Q_ASSERT(segment_number >= 0 && segment_number < (int)segment_starts.size());
	return segment_points[segment_starts[segment_number] + point_number];
}

const QString& Track::getSegmentName(int segment_number) const
{
	// NOTE: Segment names not [yet] supported by most track importers.
	if (segment_names.size() == 0)
	{
		static const QString empty_string;
		return empty_string;
	}
	
	return segment_names[segment_number];
}

int Track::getNumWaypoints() const
{
	return waypoints.size();
}

const TrackPoint& Track::getWaypoint(int number) const
{
	return waypoints[number];
}

const QString& Track::getWaypointName(int number) const
{
	return waypoint_names[number];
}

LatLon Track::calcAveragePosition() const
{
	double avg_latitude = 0;
	double avg_longitude = 0;
	int num_samples = 0;
	
	int size = getNumWaypoints();
	for (int i = 0; i < size; ++i)
	{
		const TrackPoint& point = getWaypoint(i);
		avg_latitude += point.gps_coord.latitude();
		avg_longitude += point.gps_coord.longitude();
		++num_samples;
	}
	for (int i = 0; i < getNumSegments(); ++i)
	{
		size = getSegmentPointCount(i);
		for (int k = 0; k < size; ++k)
		{
			const TrackPoint& point = getSegmentPoint(i, k);
			avg_latitude += point.gps_coord.latitude();
			avg_longitude += point.gps_coord.longitude();
			++num_samples;
		}
	}
	
	return LatLon((num_samples > 0) ? (avg_latitude / num_samples) : 0,
				  (num_samples > 0) ? (avg_longitude / num_samples) : 0);
}

bool Track::loadFromGPX(QFile* file, bool project_points, QWidget* dialog_parent)
{
	Q_UNUSED(dialog_parent);
	
	track_crs = new Georeferencing();
	track_crs->setProjectedCRS("", geographic_crs_spec);
	track_crs->setTransformationDirectly(QTransform());
	
	TrackPoint point;
	QString point_name;

	QXmlStreamReader stream(file);
	while (!stream.atEnd())
	{
		stream.readNext();
		if (stream.tokenType() == QXmlStreamReader::StartElement)
		{
			if (stream.name().compare("wpt", Qt::CaseInsensitive) == 0 ||
				stream.name().compare("trkpt", Qt::CaseInsensitive) == 0 ||
				stream.name().compare("rtept", Qt::CaseInsensitive) == 0)
			{
				point = TrackPoint(LatLon(stream.attributes().value("lat").toString().toDouble(),
				                          stream.attributes().value("lon").toString().toDouble()));
				if (project_points)
					point.map_coord = map_georef.toMapCoordF(point.gps_coord); // TODO: check for errors
				point_name = "";
			}
			else if (stream.name().compare("trkseg", Qt::CaseInsensitive) == 0 ||
				stream.name().compare("rte", Qt::CaseInsensitive) == 0)
			{
				if (segment_starts.size() == 0 ||
					segment_starts.back() < (int)segment_points.size())
				{
					segment_starts.push_back(segment_points.size());
				}
			}
			else if (stream.name().compare("ele", Qt::CaseInsensitive) == 0)
				point.elevation = stream.readElementText().toFloat();
			else if (stream.name().compare("time", Qt::CaseInsensitive) == 0)
				point.datetime = QDateTime::fromString(stream.readElementText(), Qt::ISODate);
			else if (stream.name().compare("sat", Qt::CaseInsensitive) == 0)
				point.num_satellites = stream.readElementText().toInt();
			else if (stream.name().compare("hdop", Qt::CaseInsensitive) == 0)
				point.hDOP = stream.readElementText().toFloat();
			else if (stream.name().compare("name", Qt::CaseInsensitive) == 0)
				point_name = stream.readElementText();
		}
		else if (stream.tokenType() == QXmlStreamReader::EndElement)
		{
			if (stream.name().compare("wpt", Qt::CaseInsensitive) == 0)
			{
				waypoints.push_back(point);
				waypoint_names.push_back(point_name);
			}
			else if (stream.name().compare("trkpt", Qt::CaseInsensitive) == 0 ||
				stream.name().compare("rtept", Qt::CaseInsensitive) == 0)
			{
				segment_points.push_back(point);
			}
		}
	}
	
	if (segment_starts.size() > 0 &&
		segment_starts.back() == (int)segment_points.size())
	{
		segment_starts.pop_back();
	}
	
	return true;
}

bool Track::loadFromDXF(QFile* file, bool project_points, QWidget* dialog_parent)
{
	DXFParser* parser = new DXFParser();
	parser->setData(file);
	QString result = parser->parse();
	if (!result.isEmpty())
	{
		QMessageBox::critical(dialog_parent, TemplateTrack::tr("Error reading"), TemplateTrack::tr("There was an error reading the DXF file %1:\n\n%2").arg(file->fileName(), result));
		delete parser;
		return false;
	}
	QList<DXFPath> paths = parser->getData();
	delete parser;
	
	// TODO: Re-implement the possibility to load degree values somewhere else.
	//       It does not fit here as this method is called again every time a map
	//       containing a track is re-loaded, and in this case the question should
	//       not be asked again.
	//int res = QMessageBox::question(dialog_parent, TemplateTrack::tr("Question"), TemplateTrack::tr("Are the coordinates in the DXF file in degrees?"), QMessageBox::Yes|QMessageBox::No);
	foreach (DXFPath path, paths)
	{
		if (path.type == POINT)
		{
			if(path.coords.size() < 1)
				continue;
			TrackPoint point = TrackPoint(LatLon(path.coords.at(0).y, path.coords.at(0).x));
			if (project_points)
				point.map_coord = map_georef.toMapCoordF(track_crs, fakeMapCoordF(point.gps_coord)); // TODO: check for errors
			waypoints.push_back(point);
			waypoint_names.push_back(path.layer);
		}
		if (path.type == LINE ||
			path.type == SPLINE	)
		{
			if (path.coords.size() < 1)
				continue;
			segment_starts.push_back(segment_points.size());
			segment_names.push_back(path.layer);
			int i = 0;
			foreach(DXFCoordinate coord, path.coords)
			{
				TrackPoint point = TrackPoint(LatLon(coord.y, coord.x), QDateTime());
				if (project_points)
					point.map_coord = map_georef.toMapCoordF(track_crs, fakeMapCoordF(point.gps_coord)); // TODO: check for errors
				if (path.type == SPLINE &&
					i % 3 == 0 &&
					i < path.coords.size() - 3)
					point.is_curve_start = true;
					
				segment_points.push_back(point);
				++i;
			}
			if (path.closed && !segment_points.empty())
			{
				const TrackPoint& start = segment_points[segment_starts.back()];
				if (start.gps_coord != segment_points.back().gps_coord)
				{
					segment_points.push_back(start);
					segment_points.back().is_curve_start = false;
				}
			}
		}
	}
	
	return true;
}

bool Track::loadFromOSM(QFile* file, bool project_points, QWidget* dialog_parent)
{
	track_crs = new Georeferencing();
	track_crs->setProjectedCRS("", geographic_crs_spec);
	track_crs->setTransformationDirectly(QTransform());
	
	// Basic OSM file support
	// Reference: http://wiki.openstreetmap.org/wiki/OSM_XML
	const double min_supported_version = 0.5;
	const double max_supported_version = 0.6;
	QHash<QString, TrackPoint> nodes;
	int node_problems = 0;
	
	QXmlStreamReader xml(file);
	if (xml.readNextStartElement())
	{
		if (xml.name() != "osm")
		{
			QMessageBox::critical(dialog_parent, TemplateTrack::tr("Error"), TemplateTrack::tr("%1:\nNot an OSM file."));
			return false;
		}
		else
		{
			QXmlStreamAttributes attributes(xml.attributes());
			const double osm_version = attributes.value("version").toString().toDouble();
			if (osm_version < min_supported_version)
			{
				QMessageBox::critical(dialog_parent, TemplateTrack::tr("Error"), TemplateTrack::tr("The OSM file has version %1.\nThe minimum supported version is %2.").arg(attributes.value("version").toString(), QString::number(min_supported_version, 'g', 1)));
				return false;
			}
			if (osm_version > max_supported_version)
			{
				QMessageBox::critical(dialog_parent, TemplateTrack::tr("Error"), TemplateTrack::tr("The OSM file has version %1.\nThe maximum supported version is %2.").arg(attributes.value("version").toString(), QString::number(min_supported_version, 'g', 1)));
				return false;
			}
		}
	}
	
	qint64 internal_node_id = 0;
	while (xml.readNextStartElement())
	{
		const QStringRef name(xml.name());
		QXmlStreamAttributes attributes(xml.attributes());
		if (attributes.value("visible") == "false")
		{
			xml.skipCurrentElement();
			continue;
		}
		
		QString id(attributes.value("id").toString());
		if (id.isEmpty())
		{
			id = "!" + QString::number(++internal_node_id);
		}
		
		if (name == "node")
		{
			bool ok = true;
			double lat = 0.0, lon = 0.0;
			if (ok) lat = attributes.value("lat").toString().toDouble(&ok);
			if (ok) lon = attributes.value("lon").toString().toDouble(&ok);
			if (!ok)
			{
				node_problems++;
				xml.skipCurrentElement();
				continue;
			}
			
			TrackPoint point(LatLon(lat, lon));
			if (project_points)
			{
				point.map_coord = map_georef.toMapCoordF(point.gps_coord); // TODO: check for errors
			}
			nodes.insert(id, point);
			
			while (xml.readNextStartElement())
			{
				if (xml.name() == "tag")
				{
					const QString k(xml.attributes().value("k").toString());
					const QString v(xml.attributes().value("v").toString());
					element_tags[id][k] = v;
					
					if (k == "ele")
					{
						bool ok;
						double elevation = v.toDouble(&ok);
						if (ok) nodes[id].elevation = elevation;
					}
					else if (k == "name")
					{
						if (!v.isEmpty() && !nodes.contains(v)) 
						{
							waypoints.push_back(point);
							waypoint_names.push_back(v);
						}
					}
				}
				xml.skipCurrentElement();
			}
		}
		else if (name == "way")
		{
			segment_starts.push_back(segment_points.size());
			segment_names.push_back(id);
			while (xml.readNextStartElement())
			{
				if (xml.name() == "nd")
				{
					QString ref = xml.attributes().value("ref").toString();
					if (ref.isEmpty() || !nodes.contains(ref))
						node_problems++;
					else
						segment_points.push_back(nodes[ref]);
				}
				else if (xml.name() == "tag")
				{
					const QString k(xml.attributes().value("k").toString());
					const QString v(xml.attributes().value("v").toString());
					element_tags[id][k] = v;
				}
				xml.skipCurrentElement();
			}
		}
		else
		{
			xml.skipCurrentElement();
		}
	}
	
	if (node_problems > 0)
		QMessageBox::warning(dialog_parent, TemplateTrack::tr("Problems"), TemplateTrack::tr("%1 nodes could not be processed correctly.").arg(node_problems));
	
	return true;
}

void Track::projectPoints()
{
	if (track_crs->getProjectedCRSSpec() == geographic_crs_spec)
	{
		int size = waypoints.size();
		for (int i = 0; i < size; ++i)
			waypoints[i].map_coord = map_georef.toMapCoordF(waypoints[i].gps_coord, NULL); // FIXME: check for errors
			
		size = segment_points.size();
		for (int i = 0; i < size; ++i)
			segment_points[i].map_coord = map_georef.toMapCoordF(segment_points[i].gps_coord, NULL); // FIXME: check for errors
	}
	else
	{
		int size = waypoints.size();
		for (int i = 0; i < size; ++i)
			waypoints[i].map_coord = map_georef.toMapCoordF(track_crs, fakeMapCoordF(waypoints[i].gps_coord), NULL); // FIXME: check for errors
			
		size = segment_points.size();
		for (int i = 0; i < size; ++i)
			segment_points[i].map_coord = map_georef.toMapCoordF(track_crs, fakeMapCoordF(segment_points[i].gps_coord), NULL); // FIXME: check for errors
	}
}
