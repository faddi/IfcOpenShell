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

#include <pqxx/pqxx>

#include <unordered_map>
#include <map>
#include <iterator>

#include <boost/optional.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "IfcTypes.h"

// #include <boost/asio/io_service.hpp>
// #include <boost/bind.hpp>
// #include <boost/thread/thread.hpp>


#if USE_VLD
#include <vld.h>
#endif

using namespace IfcSchema;

namespace importer {

    const std::string DATA_TABLE = "ifctable";
    const std::string DATA_IFCTYPE = "ifctypes";

    typedef std::map<std::string, std::string> ssMap;
    typedef rapidjson::Value jValue;
    typedef rapidjson::Document jDoc;


    /*
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
    */


    void findEnt ( std::string type, IfcParse::IfcFile& file) {

        auto list = file.entitiesByType(type);

        int size = 0;

        if (list != NULL) {
            size = list->size();
        }

        std::cout << "Found " << size << " " << type << "\n";

        list.reset();
    }


    /*
      void traverseProject (IfcParse::IfcFile& file) {

      IfcEntityList::ptr projects = file.entitiesByType(IfcSchema::Type::IfcProject);

      assert(projects != NULL);
      assert(projects->size() == 1);

      IfcSchema::IfcProject* p = static_cast<IfcSchema::IfcProject*>(*projects->begin());

      auto decomp = p->IsDecomposedBy();
      }
    */

    void print_type(IfcUtil::IfcBaseClass* instance) {
        std::cout << IfcSchema::Type::ToString(instance->type()) << std::endl;
    }

    // Format an IFC attribute and maybe returns as string. Only literal scalar
    // values are converted. Things like entity instances and lists are omitted.
    jValue format_attribute(const Argument* argument, IfcUtil::ArgumentType argument_type, const std::string& argument_name, rapidjson::Document::AllocatorType& alloc) {
        jValue value;

        // Hard-code lat-lon as it represents an array
        // of integers best emitted as a single decimal
        if (argument_name == "IfcSite.RefLatitude" ||
            argument_name == "IfcSite.RefLongitude")
        {
            std::vector<int> angle = *argument;
            double deg;
            if (angle.size() >= 3) {
                deg = angle[0] + angle[1] / 60. + angle[2] / 3600.;
                int prec = 8;
                if (angle.size() == 4) {
                    deg += angle[3] / (1000000. * 3600.);
                    prec = 14;
                }
                // std::stringstream stream;
                // stream << std::setprecision(prec) << deg;
                // value = stream.str();
                value = deg;
            }
            return value;
        }

        switch(argument_type) {
        case IfcUtil::Argument_BOOL: {
            const bool b = *argument;
            value = b;
            break; }
        case IfcUtil::Argument_DOUBLE: {
            const double d = *argument;
            value = d;
            // std::stringstream stream;
            // stream << d;
            // value = stream.str();
            break; }
        case IfcUtil::Argument_STRING:
        case IfcUtil::Argument_ENUMERATION: {
            // value = static_cast<std::string>(*argument);
            value = jValue(static_cast<std::string>(*argument).c_str(), alloc);
            break; }
        case IfcUtil::Argument_INT: {
            const int v = *argument;
            value = v;
            // std::stringstream stream;
            // stream << v;
            // value = stream.str();
            break; }
        case IfcUtil::Argument_ENTITY_INSTANCE: {
            IfcUtil::IfcBaseClass* e = *argument;
            if (Type::IsSimple(e->type())) {
                IfcUtil::IfcBaseType* f = (IfcUtil::IfcBaseType*) e;
                value = format_attribute(f->getArgument(0), f->getArgumentType(0), argument_name, alloc);
            } else if (e->is(IfcSchema::Type::IfcSIUnit) || e->is(IfcSchema::Type::IfcConversionBasedUnit)) {
                // Some string concatenation to have a unit name as a XML attribute.

                std::string unit_name;

                if (e->is(IfcSchema::Type::IfcSIUnit)) {
                    IfcSchema::IfcSIUnit* unit = (IfcSchema::IfcSIUnit*) e;
                    unit_name = IfcSchema::IfcSIUnitName::ToString(unit->Name());
                    if (unit->hasPrefix()) {
                        unit_name = IfcSchema::IfcSIPrefix::ToString(unit->Prefix()) + unit_name;
                    }
                } else {
                    IfcSchema::IfcConversionBasedUnit* unit = (IfcSchema::IfcConversionBasedUnit*) e;
                    unit_name = unit->Name();
                }

                value = jValue(unit_name.c_str(), alloc);
            } else if (e->is(IfcSchema::Type::IfcLocalPlacement)) {
                IfcSchema::IfcLocalPlacement* placement = e->as<IfcSchema::IfcLocalPlacement>();
                gp_Trsf trsf;
                IfcGeom::Kernel kernel;
                if (kernel.convert(placement, trsf)) {
                    std::stringstream stream;
                    for (int i = 1; i < 5; ++i) {
                        for (int j = 1; j < 4; ++j) {
                            const double trsf_value = trsf.Value(j, i);
                            stream << trsf_value << " ";
                        }
                        stream << ((i == 4) ? "1" : "0 ");
                    }
                    value = jValue(stream.str().c_str(), alloc);
                }
            }
            break; }

        case IfcUtil::Argument_AGGREGATE_OF_INT: {
            std::vector<int> list = *argument;
            value.SetArray();

            for(auto it = list.begin(); it != list.end(); ++it) {
                value.PushBack(*it, alloc);
            }

            break; }
        case IfcUtil::Argument_AGGREGATE_OF_DOUBLE: {

            std::vector<double> list = *argument;
            value.SetArray();

            for(auto it = list.begin(); it != list.end(); ++it) {
                value.PushBack(*it, alloc);
            }
            break; }
        case IfcUtil::Argument_AGGREGATE_OF_STRING: {

            std::vector<std::string> list = *argument;
            value.SetArray();

            for(auto it = list.begin(); it != list.end(); ++it) {

                jValue v(it->c_str(), alloc);

                value.PushBack(v, alloc);
            }
            break; }
        case IfcUtil::Argument_AGGREGATE_OF_ENTITY_INSTANCE: {

            IfcEntityList::ptr list = *argument;

            jValue inner;
            inner.SetArray();

            for(auto it = list->begin(); it != list->end(); ++it) {
                inner.PushBack((*it)->entity->id(), alloc);
            }

            value.SetObject();
            value.AddMember("ref", inner, alloc);

            break; }
        default:
            Logger::Message(Logger::LOG_WARNING, "Unknown ent : " + std::string(IfcUtil::ArgumentTypeToString(argument->type())) + " - " + argument->toString());
            break;
        }
        return value;
    }



    jDoc collect_props(IfcUtil::IfcBaseClass* instance) {

        jDoc d;
        d.SetObject();

        const unsigned n = instance->getArgumentCount();
        for (unsigned i = 0; i < n; ++i) {
            const Argument* argument = instance->getArgument(i);
            if (argument->isNull()) continue;

            std::string argument_name = instance->getArgumentName(i);

            const IfcUtil::ArgumentType argument_type = instance->getArgumentType(i);

            // const std::string qualified_name = IfcSchema::Type::ToString(instance->type()) + "." + argument_name;
            const std::string qualified_name = argument_name;
            jValue value;
            try {
                value = format_attribute(argument, argument_type, qualified_name, d.GetAllocator());
            } catch (...) {}

            // t.insert(std::pair<std::string, std::string>(qualified_name, *value));

            jValue k(qualified_name.c_str(), d.GetAllocator());
            d.AddMember(k, value, d.GetAllocator());
        }

        return d;
    }



    std::string escape_quotes(const std::string &before) {
        std::string after;
        after.reserve(before.length() + 4);

        for (std::string::size_type i = 0; i < before.length(); ++i) {
            switch (before[i]) {
            case '"':
            case '\\':
                after += '\\';
                // Fall through.

            default:
                after += before[i];
            }
        }

        return after;
    }


    void generateGeometry(IfcParse::IfcFile& file, pqxx::connection& con) {


        IfcSchema::IfcGeometricRepresentationContext::list::ptr contexts = file.entitiesByType<IfcSchema::IfcGeometricRepresentationContext>();

        std::cout << contexts->size() << std::endl;

        for (auto& c : *contexts) {
            std::cout << c->entity->id() << std::endl;
        }

    }

    void doImport(IfcParse::IfcFile& file, pqxx::connection& con) {

        // findEnt("IfcProject", file);
        // findEnt("IfcBuilding", file);
        // findEnt("IfcSite", file);
        // findEnt("IfcBuildingStorey", file);

        std::stringstream s;

        s << "INSERT INTO " << con.esc(importer::DATA_TABLE) << " (id, entityId, type, properties, entity) VALUES \n";

        std::cout << "Building insert string";

        int counter = 0;

        for (auto a = file.begin(); a != file.end(); ++a) {

            IfcUtil::IfcBaseClass* i = a->second;

            // print_type(i);

            jDoc props = collect_props(i);

            rapidjson::StringBuffer buf;
            rapidjson::Writer<rapidjson::StringBuffer> w(buf);
            props.Accept(w);

            std::string json = std::string(buf.GetString());

            if (counter != 0) {
                s << ",";
            }

            s << "(default,"
              << i->entity->id() << ", "
              << "'" << con.esc(IfcSchema::Type::ToString(i->type())) << "'" << ", "
              << "'" << con.esc(json) << "'" << ", "
              << "'" << con.esc(i->entity->toString(false)) << "'"
              << ") \n";

            if (counter % 10000 == 0 && counter > 0 ) {
                std::cerr << ".";
                s << ";";
                pqxx::nontransaction W(con);
                // std::cout << s.str() << std::endl;
                W.exec( s.str().c_str() );
                s.str("");
                s << "INSERT INTO " << con.esc(importer::DATA_TABLE) << " (id, entityId, type, properties, entity) VALUES \n";
                counter = 0;
                continue;
            }

            counter++;


        }

        s << ";";
        std::cout << "Executing query." << std::endl;
        pqxx::nontransaction W(con);
        W.exec( s.str().c_str() );
        s.str("");
        s << "INSERT INTO " << con.esc(importer::DATA_TABLE) << " (id, entityId, type, properties, entity) VALUES \n";

        std::cout << "Done." << std::endl;

        // std::cout << s.str().c_str() << std::endl;

        // W.commit();

        std::cout << "Done." << std::endl;
    }



}


boost::optional<pqxx::result> db_exec (pqxx::connection& con, char const* query) {

    try {

        // Create a transactional object.
        pqxx::nontransaction W(con);

        // Execute SQL query
        pqxx::result res(W.exec( query ));

        cout << "Table aborted successfully" << endl;

        return res;

    } catch(std::exception& e) {
        std::cerr << "Error: " << std::endl;
        std::cerr << e.what() << std::endl;
        return {};
    }

}


// bool table_exists (pqxx::connection& con, std::string table_name) {
//
//     std::stringstream s;
//
//     s << "SELECT EXISTS ( "
//       << "SELECT 1"
//       << " FROM   information_schema.tables"
//       << " WHERE  table_schema = 'schema_name'"
//       << " AND    table_name = '" << con.esc(table_name) << "'"
//       << ");";
//
//     boost::optional<pqxx::result> res = db_exec(con, s.str().c_str());
//
//     if (res) {
//
//         std::cout << "result size : " << res->size() << std::endl;
//
//
//
//     }
//
//     return false;
// }

bool create_table (pqxx::connection& con) {

    try {

        std::stringstream s;

        // Create a transactional object.
        pqxx::work W(con);

        s << "DROP TABLE IF EXISTS " << con.esc(importer::DATA_TABLE) << ";";
        W.exec( s.str().c_str() );
        s.str("");

        s << "DROP TYPE IF EXISTS " << con.esc(importer::DATA_IFCTYPE) << ";";
        W.exec( s.str().c_str() );
        s.str("");

        s << "CREATE TYPE " << con.esc(importer::DATA_IFCTYPE) << " AS ENUM (" << IfcTypes << ");";
        W.exec( s.str().c_str() );
        s.str("");

        s << "CREATE TABLE " << con.esc(importer::DATA_TABLE) << "("
          << "id             serial PRIMARY KEY,"
          << "entityId       INT NOT NULL,"
          << "type           ifctypes    NOT NULL,"
          << "properties     JSONB     NOT NULL,"
          << "entity         TEXT)";

        W.exec( s.str().c_str() );

        W.commit();

        cout << "Table commited successfully" << endl;

    } catch(std::exception& e) {
        std::cerr << "Error: " << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
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


    try {
        pqxx::connection con("dbname=strusoft user=strusoft password=strusoft hostaddr=127.0.0.1 port=5432");

        if (con.is_open()) {
            std::cout << "Opened database successfully: " << con.dbname() << std::endl;
        } else {
            std::cout << "Can't open database" << std::endl;
            return 1;
        }

        importer::generateGeometry(file, con);

        // if (!create_table(con)) {
        //     std::cout << "Couldn't create table";
        //     return 1;
        // }

        // importer::doImport(file, con);

        con.disconnect ();
        std::cout << "Connection closed." << std::endl;


    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }




}




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
