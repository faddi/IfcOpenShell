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

#include "../ifcgeom/IfcGeom.h"
#include "../ifcgeom/IfcGeomIterator.h"

#include <unordered_map>
#include <vector>
#include <string>

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#if USE_VLD
#include <vld.h>
#endif

using namespace IfcSchema;


void fetchGeom (IfcGeom::Iterator<double> &contextIterator) {

	boost::asio::io_service ioService;
	boost::thread_group threadpool;

	boost::asio::io_service::work work(ioService);

	// add threads
	for (int i = 0; i < 8; i++) {
		threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
	}

	std::vector<std::string> allIds;
	unsigned int total = 0;

	do
	{

		total++;
		ioService.post(boost::bind<void>([&]{
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


				}));



	} while (contextIterator.next());

	std::cout << "--------------------------------------------------------------------------------";
	std::cout << "LAST ADDED\n";

	ioService.stop();
	threadpool.join_all();

	std::cout << "Total: " << total << "\n";
}


// void fetchGeom (IfcGeom::Iterator<double> &contextIterator) {
//
//
// 	std::vector<std::string> allIds;
// 	unsigned int total = 0;
//
// 	do
// 	{
// 		IfcGeom::Element<double> *ob = contextIterator.get();
// 		auto ob_geo = static_cast<const IfcGeom::TriangulationElement<double>*>(ob);
// 		std::cout << "--------------------------------------------------------------------------------\n";
// 		std::cout << "Type: " << ob->type() << "\n";
// 		std::cout << "Id: " << ob->id() << "\n";
// 		std::cout << "Name: " << ob->name() << "\n";
// 		total++;
//
// 		if (ob_geo)
// 		{
// 			auto faces = ob_geo->geometry().faces();
// 			auto vertices = ob_geo->geometry().verts();
// 			auto normals = ob_geo->geometry().normals();
// 			auto uvs = ob_geo->geometry().uvs();
//
// 			std::cout << "Vertices: " << vertices.size() << "\n";
// 			std::cout << "Faces: " << faces.size() << "\n";
//
// 			// for (unsigned int i = 0; i < vertices.size(); i++) {
// 			// 	auto v = vertices.at(i);
//
//
//
// 			// }
//
//
//
//
//
// 			// std::unordered_map<int, std::unordered_map<int, int>>  indexMapping;
// 			// std::unordered_map<int, int> vertexCount;
// 			// std::unordered_map<int, std::vector<double>> post_vertices, post_normals, post_uvs;
// 			// std::unordered_map<int, std::vector<repo_face_t>> post_faces;
//
// 			// auto matIndIt = ob_geo->geometry().material_ids().begin();
//
// 			// for (int i = 0; i < vertices.size(); i += 3)
// 			// {
// 			// 	for (int j = 0; j < 3; ++j)
// 			// 	{
// 			// 		int index = j + i;
// 			// 		if (offset.size() < j + 1)
// 			// 		{
// 			// 			offset.push_back(vertices[index]);
// 			// 		}
// 			// 		else
// 			// 		{
// 			// 			offset[j] = offset[j] > vertices[index] ? vertices[index] : offset[j];
// 			// 		}
// 			// 	}
// 			// }
//
// 			// for (int iface = 0; iface < faces.size(); iface += 3)
// 			// {
// 			// 	auto matInd = *matIndIt;
// 			// 	if (indexMapping.find(matInd) == indexMapping.end())
// 			// 	{
// 			// 		//new material
// 			// 		indexMapping[matInd] = std::unordered_map<int, int>();
// 			// 		vertexCount[matInd] = 0;
//
// 			// 		std::unordered_map<int, std::vector<double>> post_vertices, post_normals, post_uvs;
// 			// 		std::unordered_map<int, std::vector<repo_face_t>> post_faces;
//
// 			// 		post_vertices[matInd] = std::vector<double>();
// 			// 		post_normals[matInd] = std::vector<double>();
// 			// 		post_uvs[matInd] = std::vector<double>();
// 			// 		post_faces[matInd] = std::vector<repo_face_t>();
//
// 			// 		auto material = ob_geo->geometry().materials()[matInd];
// 			// 		std::string matName = useMaterialNames ? material.original_name() : material.name();
// 			// 		allMaterials.push_back(matName);
// 			// 		if (materials.find(matName) == materials.end())
// 			// 		{
// 			// 			//new material, add it to the vector
// 			// 			repo_material_t matProp = createMaterial(material);
// 			// 			materials[matName] = new repo::core::model::MaterialNode(repo::core::model::RepoBSONFactory::makeMaterialNode(matProp, matName));
// 			// 		}
// 			// 	}
//
// 			// 	repo_face_t face;
// 			// 	for (int j = 0; j < 3; ++j)
// 			// 	{
// 			// 		auto vIndex = faces[iface + j];
// 			// 		if (indexMapping[matInd].find(vIndex) == indexMapping[matInd].end())
// 			// 		{
// 			// 			//new index. create a mapping
// 			// 			indexMapping[matInd][vIndex] = vertexCount[matInd]++;
// 			// 			for (int ivert = 0; ivert < 3; ++ivert)
// 			// 			{
// 			// 				auto bufferInd = ivert + vIndex * 3;
// 			// 				post_vertices[matInd].push_back(vertices[bufferInd]);
//
// 			// 				if (normals.size())
// 			// 					post_normals[matInd].push_back(normals[bufferInd]);
//
// 			// 				if (uvs.size() && ivert < 2)
// 			// 				{
// 			// 					auto uvbufferInd = ivert + vIndex * 2;
// 			// 					post_uvs[matInd].push_back(uvs[uvbufferInd]);
// 			// 				}
// 			// 			}
// 			// 		}
// 			// 		face.push_back(indexMapping[matInd][vIndex]);
// 			// 	}
//
// 			// 	post_faces[matInd].push_back(face);
//
// 			// 	++matIndIt;
// 			// }
//
// 			// auto guid = ob_geo->guid();
// 			// auto name = ob_geo->name();
// 			// for (const auto& pair : post_faces)
// 			// {
// 			// 	auto index = pair.first;
// 			// 	allVertices.push_back(post_vertices[index]);
// 			// 	allNormals.push_back(post_normals[index]);
// 			// 	allFaces.push_back(pair.second);
// 			// 	allUVs.push_back(post_uvs[index]);
//
// 			// 	allIds.push_back(guid);
// 			// 	allNames.push_back(name);
// 			// }
// 		}
// 		// if (allIds.size() % 100 == 0)
// 		// 	repoInfo << allIds.size() << " meshes created";
// 	} while (contextIterator.next());
//
// 	std::cout << "Total: " << total << "\n";
//
// }



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


	// try to find sites
	auto root = file.entitiesByType(IfcSchema::Type::IfcProduct);

	// if no sites are found try buildings
	// if (root == NULL) {
	// 	std::cout << "Could not find any sites. Trying buildings";
	// 	root = file.entitiesByType(IfcSchema::Type::IfcBuilding);
	// }

	if (root == NULL) {
		std::cout << "Could not find any buildings";
		return 0;
	}

	std::cout << "Found " << root->size() << " sites.\n";

	for (auto &elem: *root) {

		IfcGeom::IteratorSettings settings;
		settings.set(IfcGeom::IteratorSettings::APPLY_DEFAULT_MATERIALS, false);
		settings.set(IfcGeom::IteratorSettings::DISABLE_OPENING_SUBTRACTIONS, false);
		settings.set(IfcGeom::IteratorSettings::NO_NORMALS, true);
		settings.set(IfcGeom::IteratorSettings::WELD_VERTICES, false);
		// settings.set(IfcGeom::IteratorSettings::DISABLE_TRIANGULATION, true);
		settings.set(IfcGeom::IteratorSettings::FASTER_BOOLEANS, true);
		settings.set(IfcGeom::IteratorSettings::GENERATE_UVS, false);
		settings.set(IfcGeom::IteratorSettings::USE_WORLD_COORDS, true);
		settings.set(IfcGeom::IteratorSettings::SEW_SHELLS, true);

		IfcGeom::Iterator<double> contextIterator(settings, &file);

		if (contextIterator.initialize() == false) {
			std::cout << "failed to initialize contextIterator.\n";
			return 1;
		}

		std::cout << "Initialized contextIterator.\n";


		std::cout << "id : " << elem->entity->id() << "\n";

		fetchGeom(contextIterator);

	}

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
