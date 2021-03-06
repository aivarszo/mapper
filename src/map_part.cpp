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


#include "map_part.h"

#include <algorithm>

#include <qmath.h>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "file_format.h"
#include "map.h"
#include "object.h"
#include "object_operations.h"
#include "object_undo.h"
#include "renderable.h"
#include "util.h"
#include "util/xml_stream_util.h"


namespace literal
{
	const QLatin1String part("part");
	const QLatin1String name("name");
	const QLatin1String objects("objects");
	const QLatin1String object("object");
	const QLatin1String count("count");
}


MapPart::MapPart(const QString& name, Map* map)
: name(name)
, map(map)
{
	Q_ASSERT(map);
	; // nothing else
}

MapPart::~MapPart()
{
	for (Object* object : objects)
		delete object;
}


void MapPart::setName(const QString new_name)
{
	name = new_name;
	if (map)
		emit map->mapPartChanged(map->findPartIndex(this), this);
}


#ifndef NO_NATIVE_FILE_FORMAT

bool MapPart::load(QIODevice* file, int version, Map* map)
{
	loadString(file, name);
	
	int size;
	file->read((char*)&size, sizeof(int));
	objects.resize(size, nullptr);
	
	for (Object*& object : objects)
	{
		int save_type;
		file->read((char*)&save_type, sizeof(int));
		object = Object::getObjectForType(static_cast<Object::Type>(save_type), nullptr);
		if (!object)
			return false;
		object->load(file, version, map);
	}
	return true;
}

#endif

void MapPart::save(QXmlStreamWriter& xml) const
{
	XmlElementWriter part_element(xml, literal::part);
	part_element.writeAttribute(literal::name, name);
	{
		XmlElementWriter objects_element(xml, literal::objects);
		std::size_t size = objects.size();
		objects_element.writeAttribute(literal::count, size);
		for (const Object* object : objects)
		{
			object->save(xml);
		}
	}
}

MapPart* MapPart::load(QXmlStreamReader& xml, Map& map, SymbolDictionary& symbol_dict)
{
	Q_ASSERT(xml.name() == literal::part);
	
	XmlElementReader part_element(xml);
	MapPart* part = new MapPart(part_element.attribute<QString>(literal::name), &map);
	
	while (xml.readNextStartElement())
	{
		if (xml.name() == literal::objects)
		{
			XmlElementReader objects_element(xml);
			
			std::size_t num_objects = objects_element.attribute<std::size_t>(literal::count);
			if (num_objects > 0)
				part->objects.reserve(qMin(num_objects, (std::size_t)50)); // 50 is not a limit
			
			while (xml.readNextStartElement())
			{
				if (xml.name() == literal::object)
					part->objects.push_back(Object::load(xml, &map, symbol_dict));
				else
					xml.skipCurrentElement(); // unknown
			}
		}
		else
			xml.skipCurrentElement(); // unknown
	}
	
	return part;
}

int MapPart::findObjectIndex(const Object* object) const
{
	int size = objects.size();
	for (int i = size - 1; i >= 0; --i)
	{
		if (objects[i] == object)
			return i;
	}
//	Q_ASSERT(false);
	return -1;
}

void MapPart::setObject(Object* object, int pos, bool delete_old)
{
	map->removeRenderablesOfObject(objects[pos], true);
	if (delete_old)
		delete objects[pos];
	
	objects[pos] = object;
	object->setMap(map);
	object->update();
	map->setObjectsDirty(); // TODO: remove from here, dirty state handling should be separate
}

void MapPart::addObject(Object* object, int pos)
{
	objects.insert(objects.begin() + pos, object);
	object->setMap(map);
	object->update();
	
	if (objects.size() == 1 && map->getNumObjects() == 1)
		map->updateAllMapWidgets();
}

void MapPart::deleteObject(int pos, bool remove_only)
{
	map->removeRenderablesOfObject(objects[pos], true);
	if (remove_only)
		objects[pos]->setMap(nullptr);
	else
		delete objects[pos];
	objects.erase(objects.begin() + pos);
	
	if (objects.empty() && map->getNumObjects() == 0)
		map->updateAllMapWidgets();
}

bool MapPart::deleteObject(Object* object, bool remove_only)
{
	int size = objects.size();
	for (int i = size - 1; i >= 0; --i)
	{
		if (objects[i] == object)
		{
			deleteObject(i, remove_only);
			return true;
		}
	}
	return false;
}

void MapPart::importPart(MapPart* other, QHash<const Symbol*, Symbol*>& symbol_map, bool select_new_objects)
{
	if (other->getNumObjects() == 0)
		return;
	
	bool first_objects = map->getNumObjects() == 0;
	DeleteObjectsUndoStep* undo_step = new DeleteObjectsUndoStep(map);
	if (select_new_objects)
		map->clearObjectSelection(false);
	
	objects.reserve(objects.size() + other->objects.size());
	for (const Object* object: other->objects)
	{
		Object* new_object = object->duplicate();
		if (symbol_map.contains(new_object->getSymbol()))
			new_object->setSymbol(symbol_map.value(new_object->getSymbol()), true);
		
		objects.push_back(new_object);
		new_object->setMap(map);
		new_object->update();
		
		undo_step->addObject((int)objects.size() - 1);
		if (select_new_objects)
			map->addObjectToSelection(new_object, false);
	}
	
	map->push(undo_step);
	map->setObjectsDirty();
	if (select_new_objects)
	{
		map->emitSelectionChanged();
		map->emitSelectionEdited();		// TODO: is this necessary here?
		                                // Not as long as observers listen to both...
	}
	if (first_objects)
		map->updateAllMapWidgets();
}

void MapPart::findObjectsAt(
        MapCoordF coord,
        float tolerance,
        bool treat_areas_as_paths,
        bool extended_selection,
        bool include_hidden_objects,
        bool include_protected_objects,
        SelectionInfoVector& out ) const
{
	for (Object* object : objects)
	{
		if (!include_hidden_objects && object->getSymbol()->isHidden())
			continue;
		if (!include_protected_objects && object->getSymbol()->isProtected())
			continue;
		
		object->update();
		int selected_type = object->isPointOnObject(coord, tolerance, treat_areas_as_paths, extended_selection);
		if (selected_type != (int)Symbol::NoSymbol)
			out.emplace_back(selected_type, object);
	}
}

void MapPart::findObjectsAtBox(
        MapCoordF corner1,
        MapCoordF corner2,
        bool include_hidden_objects,
        bool include_protected_objects,
        std::vector< Object* >& out ) const
{
	auto rect = QRectF(corner1, corner2).normalized();
	for (Object* object : objects)
	{
		if (!include_hidden_objects && object->getSymbol()->isHidden())
			continue;
		if (!include_protected_objects && object->getSymbol()->isProtected())
			continue;
		
		object->update();
		if (rect.intersects(object->getExtent()) && object->intersectsBox(rect))
			out.push_back(object);
	}
}

int MapPart::countObjectsInRect(QRectF map_coord_rect, bool include_hidden_objects) const
{
	int count = 0;
	for (const Object* object : objects)
	{
		if (object->getSymbol()->isHidden() && !include_hidden_objects)
			continue;
		object->update();
		if (object->getExtent().intersects(map_coord_rect))
			++count;
	}
	return count;
}

QRectF MapPart::calculateExtent(bool include_helper_symbols) const
{
	QRectF rect;
	
	int i = 0;
	int size = objects.size();
	while (size > i && !rect.isValid())
	{
		if ((include_helper_symbols || !objects[i]->getSymbol()->isHelperSymbol()) && !objects[i]->getSymbol()->isHidden())
		{
			objects[i]->update();
			rect = objects[i]->getExtent();
		}
		++i;
	}
	
	for (; i < size; ++i)
	{
		if ((include_helper_symbols || !objects[i]->getSymbol()->isHelperSymbol()) && !objects[i]->getSymbol()->isHidden())
		{
			objects[i]->update();
			rectInclude(rect, objects[i]->getExtent());
		}
	}
	
	return rect;
}
