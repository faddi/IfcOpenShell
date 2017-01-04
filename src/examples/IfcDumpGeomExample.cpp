/********************************************************************************
 *                                                                              *
 * This file is part of IfcOpenShell.                                           *
 *                                                                              *
 * IfcOpenShell is free software: you can redistribute it and/or modify         *
 * it under the terms of the Lesser GNU General Public License as published by  *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * IfcOpenShell is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * Lesser GNU General Public License for more details.                          *
 *                                                                              *
 * You should have received a copy of the Lesser GNU General Public License     *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "../ifcparse/IfcFile.h"
#include "../ifcparse/IfcUtil.h"

#include "../ifcgeom/IfcGeom.h"
#include "../ifcgeom/IfcGeomIterator.h"

#include <unordered_map>

// #include <boost/asio/io_service.hpp>
// #include <boost/bind.hpp>
// #include <boost/thread/thread.hpp>


#if USE_VLD
#include <vld.h>
#endif

using namespace IfcSchema;


void fetchGeom (IfcGeom::Iterator<double> &contextIterator) {

	std::vector<std::string> allIds;
	unsigned int total = 0;

	do
	{

        total++;
        std::cout << "t: " << total << "\n";
        IfcGeom::Element<double> *ob = contextIterator.get();
        auto ob_geo = static_cast<const IfcGeom::TriangulationElement<double>*>(ob);
        // std::cout << "--------------------------------------------------------------------------------\n";
        // std::cout << "Type: " << ob->type() << "\n";
        // std::cout << "Id: " << ob->id() << "\n";
        // std::cout << "Name: " << ob->name() << "\n";


        if (ob_geo)
        {
            auto faces = ob_geo->geometry().faces();
            auto vertices = ob_geo->geometry().verts();

            // auto normals = ob_geo->geometry().normals();
            // auto uvs = ob_geo->geometry().uvs();

            // std::cout << "Vertices: " << vertices.size() << "\n";
            // std::cout << "Faces: " << faces.size() << "\n";

        }

	} while (contextIterator.next());

// std::cout << "--------------------------------------------------------------------------------";
// std::cout << "Total: " << total << "\n";
}



void findEnt ( std::string type, IfcParse::IfcFile& file) {

    auto list = file.entitiesByType(type);

    int size = 0;

    if (list != NULL) {
        size = list->size();
    }

    std::cout << "Found " << size << " " << type << "\n";

    list.reset();
}


void traverseProject (IfcParse::IfcFile& file) {

    IfcEntityList::ptr projects = file.entitiesByType(IfcSchema::Type::IfcProject);

    assert(projects != NULL);
    assert(projects->size() == 1);

    IfcSchema::IfcProject* p = static_cast<IfcSchema::IfcProject*>(*projects->begin());

    auto decomp = p->IsDecomposedBy();
}


int main(int argc, char** argv) {

	if ( argc != 2 ) {
		std::cout << "usage: IfcParseExamples <filename.ifc>" << std::endl;
		return 1;
	}

	// Redirect the output (both progress and log) to stdout
	Logger::SetOutput(&std::cout,&std::cout);

	// Parse the IFC file provided in argv[1]
	IfcParse::IfcFile file;
	if ( ! file.Init(argv[1]) ) {
		std::cout << "Unable to parse .ifc file" << std::endl;
		return 1;
	}


    findEnt("IfcProject", file);
    findEnt("IfcBuilding", file);
    findEnt("IfcSite", file);
    findEnt("IfcBuildingStorey", file);



	// // try to find sites
	// auto root = file.entitiesByType(IfcSchema::Type::IfcProject);

	// // if no sites are found try buildings
	// // if (root == NULL) {
	// // 	std::cout << "Could not find any sites. Trying buildings";
	// // 	root = file.entitiesByType(IfcSchema::Type::IfcBuilding);
	// // }

	// if (root == NULL) {
	// 	std::cout << "Could not find any buildings";
	// 	return 0;
	// }

	// std::cout << "Found " << root->size() << " sites.\n";

    // // IfcEntityList::it proj = root->begin();




    return 0;

    /*

    boost::asio::io_service ioService;
    boost::thread_group threadpool;


    boost::asio::io_service::work work(ioService);

    for (int i = 0; i < 1; i++) {
        threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
    }

	for (auto &elem: *root) {


        ioService.post(boost::bind<void>([&argv]{
                    // cout << "hello \n";



                    std::string filename( argv[1] );

                    IfcGeom::IteratorSettings settings;
                    settings.set(IfcGeom::IteratorSettings::USE_WORLD_COORDS, false);
                    settings.set(IfcGeom::IteratorSettings::WELD_VERTICES, false);
                    settings.set(IfcGeom::IteratorSettings::CONVERT_BACK_UNITS, true);
                    // settings.set(IfcGeom::IteratorSettings::APPLY_DEFAULT_MATERIALS, false);
                    // settings.set(IfcGeom::IteratorSettings::DISABLE_OPENING_SUBTRACTIONS, false);
                    // settings.set(IfcGeom::IteratorSettings::NO_NORMALS, true);
                    // settings.set(IfcGeom::IteratorSettings::WELD_VERTICES, false);
                    // // settings.set(IfcGeom::IteratorSettings::DISABLE_TRIANGULATION, true);
                    // settings.set(IfcGeom::IteratorSettings::FASTER_BOOLEANS, true);
                    // settings.set(IfcGeom::IteratorSettings::GENERATE_UVS, false);
                    // settings.set(IfcGeom::IteratorSettings::USE_WORLD_COORDS, true);
                    // settings.set(IfcGeom::IteratorSettings::SEW_SHELLS, true);
                    // // settings.set(IfcGeom::IteratorSettings::TRAVERSE, true);

                    IfcGeom::Iterator<double> contextIterator(settings, filename);

                    if (contextIterator.initialize() == false) {
                        std::cout << "failed to initialize contextIterator.\n";
                        return 1;
                    }

                    // std::cout << "Initialized contextIterator.\n";


                    // std::cout << "id : " << elem->entity->id() << "\n";

                    fetchGeom(contextIterator);

                    // return 1;
                }));
    }

    ioService.stop();
    threadpool.join_all();


    */


	// Lets get a list of IfcBuildingElements, this is the parent
	// type of things like walls, windows and doors.
	// entitiesByType is a templated function and returns a
	// templated class that behaves like a std::vector.
	// Note that the return types are all typedef'ed as members of
	// the generated classes, ::list for the templated vector class,
	// ::ptr for a shared pointer and ::it for an iterator.
	// We will simply iterate over the vector and print a string
	// representation of the entity to stdout.
	//
	// Secondly, lets find out which of them are IfcWindows.
	// In order to access the additional properties that windows
	// have on top af the properties of building elements,
	// we need to cast them to IfcWindows. Since these properties
	// are optional we need to make sure the properties are
	// defined for the window in question before accessing them.
	// IfcBuildingElement::list::ptr elements = file.entitiesByType<IfcBuildingElement>();

	// std::cout << "Found " << elements->size() << " elements in " << argv[1] << ":" << std::endl;


	// for ( IfcBuildingElement::list::it it = elements->begin(); it != elements->end(); ++ it ) {

	// 	const IfcBuildingElement* element = *it;

	// 	element->Representation()->Representations()->begin();


	// 	// std::cout << element->entity->toString() << std::endl;

	// 	// if ( element->is(IfcBeam::Class()) ) {
	// 	// 	std::cout << "asdf";

	// 	// 	// const IfcBeam* beam = (IfcBeam*)element;


	// 	// 	// if ( window->hasOverallWidth() && window->hasOverallHeight() ) {
	// 	// 	// 	const double area = window->OverallWidth()*window->OverallHeight();
	// 	// 	// 	std::cout << "The area of this window is " << area << std::endl;
	// 	// 	// }
	// 	// }

	// }

}
