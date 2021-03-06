/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2015 Kai Pastor
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


#ifndef _OPENORIENTEERING_MAP_COORD_H_
#define _OPENORIENTEERING_MAP_COORD_H_

#include <cmath>
#include <vector>

#include <QPointF>
#include <QFlags>

class QString;
class QTextStream;
class QXmlStreamReader;
class QXmlStreamWriter;

#ifndef NO_NATIVE_FILE_FORMAT
	
/**
 * The legacy MapCoord structure, only used for legacy native file format.
 * 
 * @deprecated
 */
struct LegacyMapCoord
{
	qint64 x;
	qint64 y;
};

#endif



/**
 * Coordinates of a point in a map, with optional flags.
 * 
 * This coordinate uses what we call native map coordinates.
 * One unit in native map coordinates is 1/1000th of a millimeter on the map paper.
 * 
 * The possible flags are:
 * 
 * <ul>
 * <li><b>isCurveStart():</b> If set, this point is the first one in a sequence of
 *     four points which define a cubic bezier curve.</li>
 * <li><b>isHolePoint():</b> If set, this point marks the end of a distinct path.
 *     Only area objects may have more than one path. The first path can be
 *     understood as outline, while the remaining paths can be seen as holes in
 *     the area, subject to the filling rule.
 * <li><b>isDashPoint():</b> This flag marks special points in the path.
 *     The effect depends on the type of symbol.
 * <li><b>isClosePoint():</b> If this flag is set for the last point of a path,
 *     this path is treated as closed. The point's coordinates must be equal to
 *     the first point's coordinates in this path.</li>
 * <li><b>isGapPoint():</b> This flag is used to indicate the begin and the end
 *     of gaps in a path. This only used internally at the moment (for dealing
 *     with dashed lines.)
 * </ul>
 * 
 * These coordinates are implemented as 32 bit integers,
 * the flags are store separately. 
 */
class MapCoord
{
public:
	/**
	 * These flags provide extra information on each coordinate in a path.
	 * 
	 * Don't change the values, they are used in import/export.
	 */
	enum Flag
	{
		CurveStart = 1 << 0,
		ClosePoint = 1 << 1,
		GapPoint   = 1 << 2,
		//unused     1 << 3,
		HolePoint  = 1 << 4,
		DashPoint  = 1 << 5,
		//...
	};
	Q_DECLARE_FLAGS(Flags, Flag)
	
private:
	/** Benchmark */
	friend class CoordXmlTest;
	
	qint32 xp;
	qint32 yp;
	Flags  fp;
	
public:
	/** Creates a MapCoord with position at the origin and without any flags set. */
	constexpr MapCoord() noexcept;
	
	/** Copy constructor. */
	constexpr MapCoord(const MapCoord&) noexcept = default;
	
#ifndef NO_NATIVE_FILE_FORMAT
	
	/** Creates a MapCoord from the legacy structure.
	 * 
	 * @deprecated
	 */
	MapCoord(const LegacyMapCoord& coord);

#endif	
	
	/** Creates a MapCoord from a position given in millimeters on the map.
	 * 
	 * This is a convenience constructor for efficient construction of a point
	 * at the origin i.e. MapCoord(0, 0), or for simple vectors i.e. 
	 * MapCoord(1, 0). Intentionally, there is no public version with flags -
	 * you need to use other argument types than int if the compiler complains
	 * about ambiguity.
	 */
	constexpr MapCoord(int x, int y);
	
private:
	/** Creates a MapCoord from native map coordinates.
	 *
	 * This is exposed via fromNative() - you must be explicit when you want to
	 * use native units.
	 */
	constexpr MapCoord(qint32 x, qint32 y, int flags);
	
public:
	/** Creates a MapCoord from a position given in millimeters on the map. */
	constexpr MapCoord(qreal x, qreal y);
	
	/** Creates a MapCoord with the given flags from a position given in millimeters on the map. */
	constexpr MapCoord(qreal x, qreal y, int flags);
	
	/** Creates a MapCoord with the position taken from a QPointF. */
	explicit constexpr MapCoord(QPointF point);
	
	/** Creates a MapCoord with the given flags and with the position taken from a QPointF. */
	constexpr MapCoord(QPointF point, int flags);
	
	/** Creates a MapCoord from native map coordinates. */
	static constexpr MapCoord fromNative(qint32 x, qint32 y);
	
	/** Creates a MapCoord from native map coordinates. */
	static constexpr MapCoord fromNative(qint32 x, qint32 y, int flags);
	
	/** Assignment operator */
	MapCoord& operator= (const MapCoord& other) = default;
	
	
	/** Returns the coord's x position in millimeters on the map. */
	constexpr qreal x() const;
	
	/** Returns the coord's y position in millimeters on the map. */
	constexpr qreal y() const;
	
	/** Sets the coord's x position to a value in millimeters on the map. */
	inline void setX(qreal x);
	
	/** Sets the coord's y position to a value in millimeters on the map. */
	inline void setY(qreal y);
	
	
	/** Returns the coord's x position in native map coords. */
	constexpr qint32 nativeX() const;
	
	/** Returns the coord's y position in native map coords. */
	constexpr qint32 nativeY() const;
	
	/** Sets the coord's x position to a value in native map coords. */
	inline void setNativeX(qint32 new_x);
	
	/** Sets the coord's y position to a value in native map coords. */
	inline void setNativeY(qint32 new_y);
	
	
	/** Returns the coord's flags separately, merged into the lowest 8 bits of an int. */
	constexpr int flags() const;
	
	/** Sets the flags as retrieved by flags(). */
	void setFlags(int flags);
	
	
	/**
	 * Returns the length of this coord, seen as a vector from the origin
	 * to the given coordinate, in millimeters on the map.
	 */
	constexpr qreal length() const;
	
	/**
	 * Returns the squared length of this coord, seen as a vector from the origin
	 * to the given coordinate, in millimeters on the map. Faster than length().
	 */
	constexpr qreal lengthSquared() const;
	
	/**
	 * Returns the distance from this coord to the other
	 * in millimeters on the map.
	 */
	constexpr qreal distanceTo(const MapCoord& other) const;
	
	/**
	 * Returns the squared distance from this coord to the other
	 * in millimeters on the map. Faster than lengthTo().
	 */
	constexpr qreal distanceSquaredTo(const MapCoord& other) const;
	
	
	/**
	 * Returns if this coord's position is equal to that of the other one.
	 */
	constexpr bool isPositionEqualTo(const MapCoord& other) const;
	
	
	/** Is this point the first point of a cubic bezier curve segment? */
	constexpr bool isCurveStart() const;
	
	/** Sets the curve start flag. */
	void setCurveStart(bool value);
	
	/**
	 * Is this the last point of a closed path, which is at the same position
	 * as the first point? This is set in addition to isHolePoint().
	 */
	constexpr bool isClosePoint() const;
	
	/** Sets the close point flag. */
	void setClosePoint(bool value);
	
	
	/** Is this the start of a hole for a line? */
	constexpr bool isHolePoint() const;
	
	/** Sets the hole point flag. */
	void setHolePoint(bool value);
	
	
	/** Is this coordinate a special dash point? */
	constexpr bool isDashPoint() const;
	
	/** Sets the dash point flag. */
	void setDashPoint(bool value);
	
	
	/** Is this coordinate a gap point? */
	constexpr bool isGapPoint() const;
	
	/** Sets the gap point flag. */
	void setGapPoint(bool value);
	
	
	/** Additive inverse. */
	constexpr MapCoord operator-() const;
	
	/** Component-wise addition. */
	MapCoord& operator+= (const MapCoord& rhs_vector);
	
	/** Component-wise addition of this and a MapCoordF/QPointF. */
	MapCoord& operator+= (const QPointF& rhs_vector);
	
	/** Component-wise subtraction. */
	MapCoord& operator-= (const MapCoord& rhs_vector);
	
	/** Component-wise subtraction of this and a MapCoordF/QPointF. */
	MapCoord& operator-= (const QPointF& rhs_vector);
	
	/** Multiply with scalar factor. */
	MapCoord& operator*= (qreal factor);
	
	/** Divide by scalar factor. */
	MapCoord& operator/= (qreal scalar);
	
	
	/** Converts the coord's position to a QPointF. */
	constexpr explicit operator QPointF() const;
	
	
	/**
	 * Writes raw coordinates and flags to a string.
	 */
	QString toString() const;
	
	
	/** Saves the MapCoord in xml format to the stream. */
	void save(QXmlStreamWriter& xml) const;
	
	/** Loads the MapCoord in xml format from the stream. */
	static MapCoord load(QXmlStreamReader& xml);
	
	
	friend constexpr bool operator==(const MapCoord& lhs, const MapCoord& rhs);
	friend constexpr MapCoord operator+(const MapCoord& lhs, const MapCoord& rhs);
	friend constexpr MapCoord operator+(const MapCoord& lhs, const QPointF& rhs);
	friend constexpr MapCoord operator-(const MapCoord& lhs, const MapCoord& rhs);
	friend constexpr MapCoord operator-(const MapCoord& lhs, const QPointF& rhs);
	friend constexpr MapCoord operator*(const MapCoord& lhs, qreal factor);
	friend constexpr MapCoord operator*(qreal factor, const MapCoord& rhs);
	friend constexpr MapCoord operator/(const MapCoord& lhs, qreal divisor);
	friend QTextStream& operator>>(QTextStream& stream, MapCoord& coord);
};

/** Compare MapCoord for equality. */
constexpr bool operator==(const MapCoord& lhs, const MapCoord& rhs);

/** Compare MapCoord for inequality. */
constexpr bool operator!=(const MapCoord& lhs, const MapCoord& rhs);


/** Component-wise addition of MapCoord. */
constexpr MapCoord operator+(const MapCoord& lhs, const MapCoord& rhs);

/** Component-wise addition of MapCoord and MapCoordF/QPointF. */
constexpr MapCoord operator+(const MapCoord& lhs, const QPointF& rhs);

/** Component-wise subtraction of MapCoord. */
constexpr MapCoord operator-(const MapCoord& lhs, const MapCoord& rhs);

/** Component-wise subtraction of MapCoord and MapCoordF/QPointF. */
constexpr MapCoord operator-(const MapCoord& lhs, const QPointF& rhs);

/** Multiply MapCoord with scalar factor. */
constexpr MapCoord operator*(const MapCoord& lhs, qreal factor);

/** Multiply MapCoord with scalar factor. */
constexpr MapCoord operator*(qreal factor, const MapCoord& rhs);

/** Divide MapCoord by scalar factor. */
constexpr MapCoord operator/(const MapCoord& lhs, qreal divisor);


/**
 * Reads raw coordinates and flags from a text stream.
 * @see MapCoord::toString()
 */
QTextStream& operator>>(QTextStream& stream, MapCoord& coord);



/**
 * Map coordinates stored as floating point numbers.
 * 
 * The unit is millimeters on the map paper.
 * 
 * This type was initially meant as intermediate format for rendering but
 * is currently used in a wide range of functions related to editing.
 * In contrast to MapCoord, MapCoordF does not store flags.
 * 
 * The type is based on QPointF and provides additional methods for using it to
 * represent 2D vectors. (Some of the methods do have counterparts in QLineF
 * rather than QPointF.) Similar to QPointF, many operators return const values,
 * although one might argue that it is no longer good practice in C++11.
 */
class MapCoordF : public QPointF
{
public:
	
	/** Creates a MapCoordF with both values set to zero. */
	constexpr MapCoordF();
	
	/** Creates a MapCoordF with the given position in map coordinates. */
	constexpr MapCoordF(qreal x, qreal y);
	
	/** Creates a MapCoordF from a MapCoord, dropping its flags. */
	explicit constexpr MapCoordF(MapCoord coord);
	
	/** Copy constructor. */
	constexpr MapCoordF(const MapCoordF&) = default;
	
	/** Copy constructor for QPointF prototypes. */
	explicit constexpr MapCoordF(const QPointF& point);
	
	
	/** Returns a vector with the given length and angle. */
	static constexpr const MapCoordF fromPolar(qreal length, qreal angle);
	
	
	/** Assignment operator. */
	MapCoordF& operator= (const QPointF& point);
	
	
	/**
	 * Returns the length of the vector.
	 * 
	 * The value returned the from this function is the MapCoords distance to
	 * the origin of the coordinate system.
	 */
	constexpr qreal length() const;
	
	/**
	 * Returns the square of the length of the vector.
	 * 
	 * This is a slightly faster alternative to MapCoordF::length() which
	 * preserves comparability.
	 */
	constexpr qreal lengthSquared() const;
	
	/**
	 * Returns the distance of the coordinate to another coordinate.
	 */
	constexpr qreal distanceTo(const MapCoordF& to) const;
	
	/**
	 * Returns the square of the distance of this coordinate to another coordinate.
	 * 
	 * This is a silghtly faster alternative to MapCoordF::distanceTo() which
	 * preserves comparability.
	 */
	constexpr qreal distanceSquaredTo(const MapCoordF& to) const;
	
	/**
	 * Changes the length of the vector.
	 * 
	 * The MapCoordF is interpreted as a vector and adjusted to a vector of the
	 * same direction but having the given lenght.
	 * It does nothing if the vector is very close to (0, 0).
	 */
	void setLength(qreal new_length);
	
	/**
	 * Normalizes the length of the vector.
	 * 
	 * The MapCoordF is interpreted as a vector and adjusted to a vector of the
	 * same direction but length 1 (i.e. unit vector).
	 */
	void normalize();
	
	
	/**
	 * Returns the angle of the vector relative to the vector (1, 0).
	 * 
	 * The returned value is in radians, in the range range [-PI; +PI].
	 * MapCoordF { 0, 1 }.getAngle() returns +PI/2,
	 * MapCoordF { 0, -1 }.getAngle() returns -PI/2.
	 */
	constexpr qreal angle() const;
	
	/**
	 * Rotates the vector.
	 * 
	 * The argument is to be given in radians.
	 * Positive arguments result in a counter-clockwise rotation.
	 */
	void rotate(qreal angle);
	
	/**
	 * Returns a vector which is the result of rotating this vector.
	 * 
	 * The argument is to be given in radians.
	 * Positive arguments result in a counter-clockwise rotation.
	 */
	constexpr const MapCoordF rotated(qreal angle) const;
	
	/**
	 * Returns a vector with the same length that is perpendicular to this vector.
	 * 
	 * Note that in contrast to normalVector(), this function returns a
	 * perpendicular vector pointing to the right.
	 * 
	 * \todo Replace with normalVector(), similar to QLineF API.
	 */
	constexpr const MapCoordF perpRight() const;
	
	/**
	 * Returns a vector with the same length that is perpendicular to this vector.
	 * 
	 * \see QLineF::normalVector()
	 */
	constexpr const MapCoordF normalVector() const;
	
	
	/** Additive inverse */
	constexpr const MapCoordF operator-();
	
	/** Component-wise addition */
	MapCoordF& operator+= (const MapCoordF& rhs);
	
	/** Component-wise subtraction */
	MapCoordF& operator-= (const MapCoordF& rhs);
	
	/** Multiply with a scalar */
	MapCoordF& operator*= (qreal factor);
	
	/** Divide by a scalar */
	MapCoordF& operator/= (qreal divisor);
	
	using QPointF::dotProduct;
};

constexpr const MapCoordF operator+(const MapCoordF& lhs, const MapCoordF& rhs);

constexpr const MapCoordF operator-(const MapCoordF& lhs, const MapCoordF& rhs);

constexpr const MapCoordF operator*(const MapCoordF& lhs, qreal factor);
constexpr const MapCoordF operator*(qreal factor, const MapCoordF& rhs);

constexpr const MapCoordF operator/(const MapCoordF& lhs, qreal divisor);



typedef std::vector<MapCoord> MapCoordVector;
typedef std::vector<MapCoordF> MapCoordVectorF;



// ### MapCoord inline code ###

Q_DECLARE_OPERATORS_FOR_FLAGS(MapCoord::Flags)

constexpr MapCoord::MapCoord() noexcept
 : xp{ 0 }
 , yp{ 0 }
 , fp{ 0 }
{
	// nothing else
}

constexpr MapCoord::MapCoord(int x, int y)
 : xp{ x*1000 }
 , yp{ y*1000 }
 , fp{ 0 }
{
	// nothing else
}

constexpr MapCoord::MapCoord(qint32 x, qint32 y, int flags)
 : xp{ x }
 , yp{ y }
 , fp{ flags }
{
	// nothing else
}

constexpr MapCoord::MapCoord(qreal x, qreal y)
 : xp{ qRound(x*1000) }
 , yp{ qRound(y*1000) }
 , fp{ 0 }
{
	// nothing else
}

constexpr MapCoord::MapCoord(qreal x, qreal y, int flags)
 : xp{ qRound(x*1000) }
 , yp{ qRound(y*1000) }
 , fp{ flags }
{
	// nothing else
}

constexpr MapCoord::MapCoord(QPointF point)
 : MapCoord { point.x(), point.y() }
{
	// nothing else
}

constexpr MapCoord::MapCoord(QPointF point, int flags)
 : MapCoord { point.x(), point.y(), flags }
{
	// nothing else
}

constexpr MapCoord MapCoord::fromNative(qint32 x, qint32 y)
{
	return MapCoord{ x, y, Flags() };
}

constexpr MapCoord MapCoord::fromNative(qint32 x, qint32 y, int flags)
{
	return MapCoord{ x, y, flags };
}

constexpr qreal MapCoord::x() const
{
	return nativeX() / 1000.0;
}

constexpr qreal MapCoord::y() const
{
	return nativeY() / 1000.0;
}

inline
void MapCoord::setX(qreal x)
{
	this->xp = qRound64(x * 1000);
}

inline
void MapCoord::setY(qreal y)
{
	this->yp = qRound64(y * 1000);
}

constexpr qint32 MapCoord::nativeX() const
{
	return xp;
}

constexpr qint32 MapCoord::nativeY() const
{
	return yp;
}

inline
void MapCoord::setNativeX(qint32 new_x)
{
	xp = new_x;
}

inline
void MapCoord::setNativeY(qint32 new_y)
{
	yp = new_y;
}

constexpr int MapCoord::flags() const
{
	return fp;
}

inline
void MapCoord::setFlags(int flags)
{
	fp = Flags(flags);
}

constexpr qreal MapCoord::length() const
{
	return sqrt(lengthSquared());
}

constexpr qreal MapCoord::lengthSquared() const
{
	return x()*x() + y()*y();
}

constexpr qreal MapCoord::distanceTo(const MapCoord& other) const
{
	return sqrt(distanceSquaredTo(other));
}

constexpr qreal MapCoord::distanceSquaredTo(const MapCoord& other) const
{
	return (*this - other).lengthSquared();
}

constexpr bool MapCoord::isPositionEqualTo(const MapCoord& other) const
{
	return (xp == other.xp) && (yp == other.yp);
}

constexpr bool MapCoord::isCurveStart() const {
	return fp.testFlag(CurveStart);
}

inline
void MapCoord::setCurveStart(bool value)
{
	if (fp.testFlag(CurveStart) != value)
		fp ^= CurveStart;
}

constexpr bool MapCoord::isClosePoint() const
{
	return fp.testFlag(ClosePoint);
}

inline
void MapCoord::setClosePoint(bool value)
{
	if (fp.testFlag(ClosePoint) != value)
		fp ^= ClosePoint;
}

constexpr bool MapCoord::isHolePoint() const
{
	return fp.testFlag(HolePoint);
}

inline
void MapCoord::setHolePoint(bool value)
{
	if (fp.testFlag(HolePoint) != value)
		fp ^= HolePoint;
}

constexpr bool MapCoord::isDashPoint() const
{
	return fp.testFlag(DashPoint);
}

inline
void MapCoord::setDashPoint(bool value)
{
	if (fp.testFlag(DashPoint) != value)
		fp ^= DashPoint;
}

constexpr bool MapCoord::isGapPoint() const
{
	return fp.testFlag(GapPoint);
}

inline
void MapCoord::setGapPoint(bool value)
{
	if (fp.testFlag(GapPoint) != value)
		fp ^= GapPoint;
}


constexpr MapCoord MapCoord::operator-() const
{
	return MapCoord { -xp, -yp, fp };
}

inline
MapCoord& MapCoord::operator+=(const MapCoord& rhs_vector)
{
	xp += rhs_vector.xp;
	yp += rhs_vector.yp;
	return *this;
}

inline
MapCoord& MapCoord::operator+=(const QPointF& rhs_vector)
{
	*this += MapCoord{ rhs_vector };
	return *this;
}

inline
MapCoord& MapCoord::operator-=(const MapCoord& rhs_vector)
{
	xp -= rhs_vector.xp;
	yp -= rhs_vector.yp;
	return *this;
}

inline
MapCoord& MapCoord::operator-=(const QPointF& rhs_vector)
{
	*this -= MapCoord{ rhs_vector };
	return *this;
}

inline
MapCoord& MapCoord::operator*=(qreal factor)
{
	xp *= factor;
	yp *= factor;
	return *this;
}

inline
MapCoord& MapCoord::operator/=(qreal divisor)
{
	xp /= divisor;
	yp /= divisor;
	return *this;
}

constexpr MapCoord::operator QPointF() const
{
	return QPointF(x(), y());
}



constexpr bool operator==(const MapCoord& lhs, const MapCoord& rhs)
{
	return (lhs.xp == rhs.xp) && (lhs.yp == rhs.yp) && (lhs.fp == rhs.fp);
}


constexpr bool operator!=(const MapCoord& lhs, const MapCoord& rhs)
{
	return !(lhs == rhs);
}


constexpr MapCoord operator+(const MapCoord& lhs, const MapCoord& rhs)
{
	return MapCoord::fromNative(lhs.xp + rhs.xp, lhs.yp + rhs.yp);
}

constexpr MapCoord operator+(const MapCoord& lhs, const QPointF& rhs)
{
	return lhs + MapCoord{ rhs };
}

constexpr MapCoord operator-(const MapCoord& lhs, const MapCoord& rhs)
{
	return MapCoord::fromNative(lhs.xp - rhs.xp, lhs.yp - rhs.yp);
}

constexpr MapCoord operator-(const MapCoord& lhs, const QPointF& rhs)
{
	return lhs - MapCoord{ rhs };
}

constexpr MapCoord operator*(const MapCoord& lhs, double factor)
{
	return MapCoord::fromNative(qRound(lhs.xp * factor), qRound(lhs.yp * factor));
}

constexpr MapCoord operator*(double factor, const MapCoord& rhs)
{
	return MapCoord::fromNative(qRound(factor * rhs.xp), qRound(factor * rhs.yp));
}

constexpr MapCoord operator/(const MapCoord& lhs, double divisor)
{
	return MapCoord::fromNative(qRound(lhs.xp / divisor), qRound(lhs.yp / divisor));
}



// ### MapCoordF inline code  ###

constexpr MapCoordF::MapCoordF()
 : QPointF {}
{
	// Nothing else
}

constexpr MapCoordF::MapCoordF(qreal x, qreal y)
 : QPointF { x, y }
{
	// Nothing else
}

constexpr MapCoordF::MapCoordF(MapCoord coord)
 : QPointF { coord.x(), coord.y() }
{
	// Nothing else
}

constexpr MapCoordF::MapCoordF(const QPointF& point)
 : QPointF { point }
{
	// Nothing else
}

// static
constexpr const MapCoordF MapCoordF::fromPolar(qreal length, qreal angle)
{
	return MapCoordF(cos(angle) * length, sin(angle) * length);
}

inline
MapCoordF& MapCoordF::operator=(const QPointF& point)
{
	return static_cast<MapCoordF&>(*static_cast<QPointF*>(this) = point);
}

constexpr qreal MapCoordF::length() const
{
	return sqrt(lengthSquared());
}

constexpr qreal MapCoordF::lengthSquared() const
{
	return x()*x() + y()*y();
}

constexpr qreal MapCoordF::distanceTo(const MapCoordF& to) const
{
	return sqrt(distanceSquaredTo(to));
}

constexpr qreal MapCoordF::distanceSquaredTo(const MapCoordF& to) const
{
	return (to - *this).lengthSquared();
}

inline
void MapCoordF::setLength(qreal new_length)
{
	auto length_squared = lengthSquared();
	if (length_squared > 1e-16)
	{
		auto factor = new_length / sqrt(length_squared);
		rx() *= factor;
		ry() *= factor;
	}
}

inline
void MapCoordF::normalize()
{
	setLength(1.0);
}

constexpr qreal MapCoordF::angle() const
{
	return atan2(y(), x());
}

inline
void MapCoordF::rotate(qreal angle)
{
	angle += this->angle();
	auto len = length();
	rx() = cos(angle) * len;
	ry() = sin(angle) * len;
}

constexpr const MapCoordF MapCoordF::rotated(qreal angle) const
{
	return MapCoordF::fromPolar(length(), angle + this->angle());
}

constexpr const MapCoordF MapCoordF::perpRight() const
{
	return MapCoordF { -y(), x() };
}

constexpr const MapCoordF MapCoordF::normalVector() const
{
	return MapCoordF { y(), -x() };
}

constexpr const MapCoordF MapCoordF::operator-()
{
	return static_cast<const MapCoordF>(-static_cast<const QPointF&>(*this));
}

inline
MapCoordF& MapCoordF::operator+=(const MapCoordF& rhs)
{
	return static_cast<MapCoordF&>(static_cast<QPointF&>(*this) += rhs);
}

inline
MapCoordF& MapCoordF::operator-=(const MapCoordF& rhs)
{
	return static_cast<MapCoordF&>(static_cast<QPointF&>(*this) -= rhs);
}

inline
MapCoordF& MapCoordF::operator*=(qreal factor)
{
	return static_cast<MapCoordF&>(static_cast<QPointF&>(*this) *= factor);
}

inline
MapCoordF& MapCoordF::operator/=(qreal divisor)
{
	return static_cast<MapCoordF&>(static_cast<QPointF&>(*this) /= divisor);
}



constexpr const MapCoordF operator+(const MapCoordF& lhs, const MapCoordF& rhs)
{
	return static_cast<const MapCoordF>(static_cast<const QPointF&>(lhs) + static_cast<const QPointF&>(rhs));
}

constexpr const MapCoordF operator-(const MapCoordF& lhs, const MapCoordF& rhs)
{
	return static_cast<const MapCoordF>(static_cast<const QPointF&>(lhs) - static_cast<const QPointF&>(rhs));
}

constexpr const MapCoordF operator*(const MapCoordF& lhs, qreal factor)
{
	return static_cast<const MapCoordF>(static_cast<const QPointF&>(lhs) * factor);
}

constexpr const MapCoordF operator*(qreal factor, const MapCoordF& rhs)
{
	return static_cast<const MapCoordF>(factor * static_cast<const QPointF&>(rhs));
}

constexpr const MapCoordF operator/(const MapCoordF& lhs, qreal divisor)
{
	return static_cast<const MapCoordF>(static_cast<const QPointF&>(lhs) / divisor);
}



#endif
