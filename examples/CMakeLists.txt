#
#    Copyright 2012, 2013 Thomas Schöps, Kai Pastor
#    
#    This file is part of OpenOrienteering.
# 
#    OpenOrienteering is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
# 
#    OpenOrienteering is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
 
set(Mapper_EXAMPLES
  overprinting.omap
  forest\ sample.omap
  sprint\ sample.omap
  complete\ map.omap
)

install(
  FILES ${Mapper_EXAMPLES}
  DESTINATION "${MAPPER_DATA_DESTINATION}/examples/${_dir}"
)

foreach(_example ${Mapper_EXAMPLES})
	configure_file(${_example} ${_example} COPYONLY)
endforeach()

