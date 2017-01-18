################################################################################
#                                                                              #
# This file is part of IfcOpenShell.                                           #
#                                                                              #
# IfcOpenShell is free software: you can redistribute it and/or modify         #
# it under the terms of the Lesser GNU General Public License as published by  #
# the Free Software Foundation, either version 3.0 of the License, or          #
# (at your option) any later version.                                          #
#                                                                              #
# IfcOpenShell is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 #
# Lesser GNU General Public License for more details.                          #
#                                                                              #
# You should have received a copy of the Lesser GNU General Public License     #
# along with this program. If not, see <http://www.gnu.org/licenses/>.         #
#                                                                              #
################################################################################

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PostgreSQL)

ADD_EXECUTABLE(IfcParseExamples IfcParseExamples.cpp)
TARGET_LINK_LIBRARIES(IfcParseExamples IfcParse)
set_target_properties(IfcParseExamples PROPERTIES FOLDER Examples)

ADD_EXECUTABLE(IfcOpenHouse IfcOpenHouse.cpp)
TARGET_LINK_LIBRARIES(IfcOpenHouse ${IFCOPENSHELL_LIBRARIES} ${OPENCASCADE_LIBRARIES})
set_target_properties(IfcOpenHouse PROPERTIES FOLDER Examples)

ADD_EXECUTABLE(IfcAdvancedHouse IfcAdvancedHouse.cpp)
TARGET_LINK_LIBRARIES(IfcAdvancedHouse ${IFCOPENSHELL_LIBRARIES} ${OPENCASCADE_LIBRARIES})
set_target_properties(IfcAdvancedHouse PROPERTIES FOLDER Examples)


include_directories(PostgreSQL_INCLUDE_DIRS)

ADD_EXECUTABLE(IfcDumpGeomExample IfcDumpGeomExample.cpp)
TARGET_LINK_LIBRARIES(IfcDumpGeomExample IfcParse ${IFCOPENSHELL_LIBRARIES} ${OPENCASCADE_LIBRARIES} ${Boost_LIBRARIES} ${PostgreSQL_LIBRARIES} "/usr/local/lib/libpqxx.dylib" "/usr/local/lib/libneo4j-client.dylib")
set_target_properties(IfcDumpGeomExample PROPERTIES FOLDER Examples)
