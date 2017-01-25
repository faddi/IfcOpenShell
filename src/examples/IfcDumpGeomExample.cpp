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

#include <neo4j-client.h>
#include <pqxx/pqxx>

#include <unordered_map>
#include <map>
#include <iterator>

// #include <boost/optional.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

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
    const std::string PROJECT_TABLE = "ifcproject";
    const std::string DATA_IFCTYPE = "ifctypes";

    typedef std::map<std::string, std::string> ssMap;
    typedef rapidjson::Value jValue;
    typedef rapidjson::Document jDoc;
    typedef double real_t;


    void fetchGeom (IfcGeom::Iterator<double> &contextIterator, pqxx::connection& con, int project) {

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

                jDoc d;
                d.SetObject();



                auto& mesh = ob_geo->geometry();

                jValue verts;
                verts.SetArray();

                for ( std::vector<real_t>::const_iterator it = mesh.verts().begin(); it != mesh.verts().end(); ) {
                    jValue v;
                    v.SetArray();
                    v.PushBack((*it++), d.GetAllocator());
                    v.PushBack((*it++), d.GetAllocator());
                    v.PushBack((*it++), d.GetAllocator());
                    verts.PushBack(v, d.GetAllocator());
                }

                d.AddMember("verts", verts, d.GetAllocator());


                jValue faces;
                faces.SetArray();

                for ( std::vector<int>::const_iterator it = mesh.faces().begin(); it != mesh.faces().end(); ) {
                    jValue v;
                    v.SetArray();

                    // const int material_id = *(material_it++);
                    // if (material_id != previous_material_id) {
                    //     const IfcGeom::Material& material = mesh.materials()[material_id];
                    //     std::string material_name = (settings().get(IfcGeom::IteratorSettings::USE_MATERIAL_NAMES)
                    //                                  ? material.original_name() : material.name());
                    //     IfcUtil::sanitate_material_name(material_name);
                    //     obj_stream << "usemtl " << material_name << "\n";
                    //     if (materials.find(material_name) == materials.end()) {
                    //         writeMaterial(material);
                    //         materials.insert(material_name);
                    //     }
                    //     previous_material_id = material_id;
                    // }

                    v.PushBack(*(it++), d.GetAllocator());
                    v.PushBack(*(it++), d.GetAllocator());
                    v.PushBack(*(it++), d.GetAllocator());

                    // if (has_normals && has_uvs) {
                    //     obj_stream << "f " << v1 << "/" << v1 << "/" << v1 << " "
                    //                << v2 << "/" << v2 << "/" << v2 << " "
                    //                << v3 << "/" << v3 << "/" << v3 << "\n";
                    // } else if (has_normals) {
                    //     obj_stream << "f " << v1 << "//" << v1 << " "
                    //                << v2 << "//" << v2 << " "
                    //                << v3 << "//" << v3 << "\n";
                    // } else {
                    //     obj_stream << "f " << v1 << " " << v2 << " " << v3 << "\n";
                    // }

                    faces.PushBack(v, d.GetAllocator());
                }
                d.AddMember("faces", faces, d.GetAllocator());

                jValue normals;
                normals.SetArray();
                for ( std::vector<real_t>::const_iterator it = mesh.normals().begin(); it != mesh.normals().end(); ) {
                    jValue v;
                    v.SetArray();

                    v.PushBack(*(it++), d.GetAllocator());
                    v.PushBack(*(it++), d.GetAllocator());
                    v.PushBack(*(it++), d.GetAllocator());

                    normals.PushBack(v, d.GetAllocator());
                }
                // d.AddMember("normals", normals, d.GetAllocator());

                jValue uvs;
                uvs.SetArray();
                for (std::vector<real_t>::const_iterator it = mesh.uvs().begin(); it != mesh.uvs().end();) {
                    jValue v;
                    v.SetArray();
                    v.PushBack(*(it++), d.GetAllocator());
                    v.PushBack(*(it++), d.GetAllocator());
                    uvs.PushBack(v, d.GetAllocator());
                }
                // d.AddMember("uvs", uvs, d.GetAllocator());


                // INSERT

                rapidjson::StringBuffer buf;
                rapidjson::Writer<rapidjson::StringBuffer> w(buf);
                d.Accept(w);

                std::string json = std::string(buf.GetString());

                std::stringstream s;

                s << "UPDATE " << con.esc(importer::DATA_TABLE) << " \n"
                  << "SET geometry = '" << con.esc(json) << "' \n"
                  << "WHERE entityid = " << ob->id() << " AND projectid = " << project << ";";

                pqxx::nontransaction W(con);
                W.exec( s.str().c_str() );
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

    template<typename T>
    void dump_json(T& d) {
        rapidjson::StringBuffer buf;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> w(buf);
        d.Accept(w);

        std::string json = std::string(buf.GetString());
        std::cout << json << std::endl;
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
            // IfcEntityList::ptr list = *argument;

            IfcUtil::IfcBaseClass* e = *argument;

            jValue inner;
            inner.SetArray();

            inner.PushBack(e->entity->id(), alloc);

            value.SetObject();
            value.AddMember("ref", inner, alloc);

            // IfcUtil::IfcBaseClass* e = *argument;
            // if (Type::IsSimple(e->type())) {
            //     IfcUtil::IfcBaseType* f = (IfcUtil::IfcBaseType*) e;
            //     value = format_attribute(f->getArgument(0), f->getArgumentType(0), argument_name, alloc);
            // } else if (e->is(IfcSchema::Type::IfcSIUnit) || e->is(IfcSchema::Type::IfcConversionBasedUnit)) {
            //     // Some string concatenation to have a unit name as a XML attribute.

            //     std::string unit_name;

            //     if (e->is(IfcSchema::Type::IfcSIUnit)) {
            //         IfcSchema::IfcSIUnit* unit = (IfcSchema::IfcSIUnit*) e;
            //         unit_name = IfcSchema::IfcSIUnitName::ToString(unit->Name());
            //         if (unit->hasPrefix()) {
            //             unit_name = IfcSchema::IfcSIPrefix::ToString(unit->Prefix()) + unit_name;
            //         }
            //     } else {
            //         IfcSchema::IfcConversionBasedUnit* unit = (IfcSchema::IfcConversionBasedUnit*) e;
            //         unit_name = unit->Name();
            //     }

            //     value = jValue(unit_name.c_str(), alloc);
            // } else if (e->is(IfcSchema::Type::IfcLocalPlacement)) {
            //     IfcSchema::IfcLocalPlacement* placement = e->as<IfcSchema::IfcLocalPlacement>();
            //     gp_Trsf trsf;
            //     IfcGeom::Kernel kernel;
            //     if (kernel.convert(placement, trsf)) {
            //         std::stringstream stream;
            //         for (int i = 1; i < 5; ++i) {
            //             for (int j = 1; j < 4; ++j) {
            //                 const double trsf_value = trsf.Value(j, i);
            //                 stream << trsf_value << " ";
            //             }
            //             stream << ((i == 4) ? "1" : "0 ");
            //         }
            //         value = jValue(stream.str().c_str(), alloc);
            //     }
            // }
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
            try {

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
            } catch (...) {}
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

    void recurseToParent(IfcUtil::IfcBaseClass* e, std::vector<IfcUtil::IfcBaseClass*>& l) {

        IfcSchema::IfcObjectDefinition* objDef = static_cast<IfcSchema::IfcObjectDefinition*>(e);
        if (!objDef) {
            return;
        }

        // IfcSchema::IfcRelDecomposes::list::ptr decompList = objDef->Decomposes();
        auto decompList = objDef->Decomposes();

        for (auto* it: *decompList) {
            // IfcTemplatedEntityList<IfcObjectDefinition>::ptr objects = it->RelatedObjects();
            // print_type(it->RelatingObject());

            recurseToParent(it->RelatingObject(), l);
            l.push_back(it->RelatingObject());
        }

        IfcSchema::IfcElement* element = static_cast<IfcSchema::IfcElement*>(e);
        if (element) {

            IfcTemplatedEntityList<IfcRelContainedInSpatialStructure>::ptr a = element->ContainedInStructure();
            for ( IfcRelContainedInSpatialStructure::list::it itt = a->begin(); itt != a->end(); ++ itt ) {
                IfcRelContainedInSpatialStructure* r = *itt;

                // print_type(r->RelatingStructure());


                recurseToParent(r->RelatingStructure(), l);
                l.push_back(r->RelatingStructure());
            }
        }
    }

    jDoc getParents(IfcUtil::IfcBaseClass* ent) {

        jDoc parents;
        parents.SetArray();

        std::vector<IfcUtil::IfcBaseClass*> l;
        recurseToParent(ent, l);
        l.push_back(ent);

        for (auto& e: l) {

            jValue parent;
            parent.SetObject();

            parent.AddMember("type", jValue(IfcSchema::Type::ToString(e->type()).c_str(), parents.GetAllocator()), parents.GetAllocator());
            jDoc props = collect_props(e);
            parent.AddMember("props", jValue(props, parents.GetAllocator()) , parents.GetAllocator());

            parents.PushBack(parent, parents.GetAllocator());
        }

        // dump_json<jDoc>(parents);
        return parents;
    }

    void generateGeometry(IfcParse::IfcFile& file, pqxx::connection& con, int project) {

        IfcGeom::IteratorSettings settings;
        settings.set(IfcGeom::IteratorSettings::USE_WORLD_COORDS, true);
        settings.set(IfcGeom::IteratorSettings::WELD_VERTICES, false);
        settings.set(IfcGeom::IteratorSettings::CONVERT_BACK_UNITS, true);
        // settings.set(IfcGeom::IteratorSettings::APPLY_DEFAULT_MATERIALS, false);
        // settings.set(IfcGeom::IteratorSettings::DISABLE_OPENING_SUBTRACTIONS, false);
        settings.set(IfcGeom::IteratorSettings::NO_NORMALS, true);
        // settings.set(IfcGeom::IteratorSettings::WELD_VERTICES, false);
        // // settings.set(IfcGeom::IteratorSettings::DISABLE_TRIANGULATION, true);
        settings.set(IfcGeom::IteratorSettings::FASTER_BOOLEANS, true);
        settings.set(IfcGeom::IteratorSettings::GENERATE_UVS, false);
        // settings.set(IfcGeom::IteratorSettings::USE_WORLD_COORDS, true);
        settings.set(IfcGeom::IteratorSettings::SEW_SHELLS, false);
        // // settings.set(IfcGeom::IteratorSettings::TRAVERSE, true);
        settings.set(IfcGeom::IteratorSettings::CENTER_MODEL, true);

        IfcGeom::Iterator<double> contextIterator(settings, &file);

        if (contextIterator.initialize() == false) {
            std::cout << "failed to initialize contextIterator.\n";
            return;
        }

        fetchGeom(contextIterator, con, project);
    }

    int doImport(IfcParse::IfcFile& file, pqxx::connection& con, std::string project) {

        std::stringstream s;

        s << "INSERT INTO " << con.esc(importer::PROJECT_TABLE) << " (id, name) VALUES (default, '" << con.esc(project) << "') RETURNING *;";

        pqxx::work W(con);
        pqxx::result res( W.exec(s.str().c_str()) );
        W.commit();

        if (res.size() != 1) {
            return -1;
        }

        int projectId = 0;

        if (res[0][0].to(projectId) == false) {
            std::cout << "Could not read project ID" << std::endl;
            return -1;
        }

        s.str("");
        s << "INSERT INTO " << con.esc(importer::DATA_TABLE) << " (id, entityId, type, properties, entity, projectId) VALUES \n";

        std::cout << "Building insert string";

        int counter = 0;

        for (auto a = file.begin(); a != file.end(); ++a) {

            IfcUtil::IfcBaseClass* i = a->second;

            if (i->is(IfcSchema::IfcProduct::Class()) == false && i->is(IfcSchema::IfcProject::Class()) == false) {
                continue;
            }

            jDoc props = collect_props(i);
            props.AddMember("parents", getParents(i), props.GetAllocator());

            rapidjson::StringBuffer buf;
            rapidjson::Writer<rapidjson::StringBuffer> w(buf);
            props.Accept(w);

            std::string json = std::string(buf.GetString());

            // std::cout << json << std::endl;

            if (counter != 0) {
                s << ",";
            }

            s << "(default,"
              << i->entity->id() << ", "
              << "'" << con.esc(IfcSchema::Type::ToString(i->type())) << "'" << ", "
              << "'" << con.esc(json) << "'" << ", "
              << "'" << con.esc(i->entity->toString(false)) << "', "
              << projectId
              << ") \n";

            if (counter % 10000 == 0 && counter > 0 ) {
                std::cerr << ".";
                s << ";";
                pqxx::nontransaction W(con);
                // std::cout << s.str() << std::endl;
                W.exec( s.str().c_str() );
                s.str("");
                s << "INSERT INTO " << con.esc(importer::DATA_TABLE) << " (id, entityId, type, properties, entity, projectId) VALUES \n";
                counter = 0;
                continue;
            }

            counter++;


        }

        s << ";";
        std::cout << "Executing query." << std::endl;
        pqxx::nontransaction W2(con);
        W2.exec( s.str().c_str() );
        // s.str("");
        // s << "INSERT INTO " << con.esc(importer::DATA_TABLE) << " (id, entityId, type, properties, entity) VALUES \n";

        // std::cout << "Done." << std::endl;

        // std::cout << s.str().c_str() << std::endl;

        // W.commit();

        std::cout << "Done." << std::endl;
        return projectId;
    }

    // NEO



    void check_err (neo4j_result_stream_t* results) {
        if (neo4j_check_failure(results) == NEO4J_STATEMENT_EVALUATION_FAILED) {
            std::string s( neo4j_error_message(results) );
            std::cout << "Error: " << s << std::endl;

            const struct neo4j_failure_details* d = neo4j_failure_details(results);

            if (d != NULL) {
                std::cout << std::endl;
                if (d->context != NULL) {
                    std::cout << std::string(d->context) << std::endl;
                }
                std::cout << std::string(d->code) << std::endl;
                std::cout << std::string(d->description) << std::endl;
                std::cout << std::string(d->message) << std::endl;
                std::cout << std::endl;
            }
        }
    }

    void print_result (neo4j_result_stream_t* result) {
        neo4j_render_table(stdout, result, 80,  0);
    }

    bool reset_neo4j (neo4j_session_t* session) {

        neo4j_result_stream_t *results = neo4j_run(session, "MATCH (n) OPTIONAL MATCH (n)-[r]-() DELETE n,r", neo4j_null);
        if (results == NULL)
        {
            neo4j_perror(stderr, errno, "Failed to run statement");
            return EXIT_FAILURE;
        }
        check_err(results);
        print_result(results);

        neo4j_close_results(results);

        return true;
    }

    bool neo_exec(neo4j_session_t& session, std::string cmd, bool dump = false) {

        if (dump) {
            std::cout << cmd << std::endl;
        }

        neo4j_result_stream_t *results = neo4j_run(&session, cmd.c_str(), neo4j_null);
        if (results == NULL) {
            neo4j_perror(stderr, errno, "Failed to run statement");
            return false;
        }
        check_err(results);
        // print_result(results);
        neo4j_close_results(results);

        return true;
    }

    void build_neo_graph(IfcParse::IfcFile& file) {

        const int bulk_size = 500;

        neo4j_client_init();

        neo4j_connection_t *connection = neo4j_connect("bolt://neo4j:asdfasdf@localhost:7687", NULL, NEO4J_INSECURE);
        if (connection == NULL)
        {
            neo4j_perror(stderr, errno, "Connection failed");
            return;
        }

        neo4j_session_t *session = neo4j_new_session(connection);
        if (session == NULL)
        {
            neo4j_perror(stderr, errno, "Failed to start session");
            return;
        }

        if (reset_neo4j(session) == false) {
            Logger::Message(Logger::LOG_ERROR, "Could not reset neo4j.");
            return;
        }

        std::cout << "Nodes." << std::endl;
        bool first = true;
        std::stringstream s;
        s << "CREATE ";
        int counter = 0;
        for (auto a = file.begin(); a != file.end(); ++a) {

            IfcUtil::IfcBaseClass* i = a->second;
            if (i->is(IfcSchema::IfcCartesianPoint::Class())) {
                continue;
            }
            // jDoc props = collect_props(i);

            // rapidjson::StringBuffer buf;
            // rapidjson::Writer<rapidjson::StringBuffer> w(buf);
            // props.Accept(w);

            // std::string json = std::string(buf.GetString());


            if (!first) {
                s << ", ";
            } else {
                first = false;
            }

            s << "(n" << counter << ":" << IfcSchema::Type::ToString(i->type()) << ":IfcNode{id:" << i->entity->id() << "}"<< ")";

            if (counter % bulk_size == 0 && counter != 0) {
                std::cerr << counter << std::endl;
                std::string cmd = s.str();
                if (neo_exec(*session, s.str()) == false) {
                    return;
                }

                s.str("");
                s << "CREATE ";
                first = true;
            }

            counter++;
        }

        if (neo_exec(*session, s.str()) == false ) {
            return;
        }

        s.str("");

        std::cout << "Relations" << std::endl;
        std::stringstream create_part;
        create_part << "CREATE ";
        s << "MATCH ";
        counter = 0;
        first = true;
        bool has_data = false;
        for (auto a = file.begin(); a != file.end(); ++a) {

            IfcUtil::IfcBaseClass* i = a->second;

            if (i->is(IfcSchema::IfcCartesianPoint::Class())) {
                continue;
            }

            int currentId = i->entity->id();

            jDoc props = collect_props(i);
            rapidjson::StringBuffer buf;
            rapidjson::Writer<rapidjson::StringBuffer> w(buf);
            props.Accept(w);

            std::string json = std::string(buf.GetString());
            // std::cout << json << std::endl;


            for (jValue::ConstMemberIterator itr = props.MemberBegin(); itr != props.MemberEnd(); ++itr) {

                if (itr->value.IsObject() && itr->value.HasMember("ref")) {
                    for (auto& v: itr->value["ref"].GetArray()) {
                        // std::cout << v.GetInt() << std::endl;
                        // std::cout << currentId << " -> " << itr->name.GetString() << " -> " << v.GetInt() << std::endl;
                        //  || i->is(IfcSchema::IfcPolyLoop::Class())

                        // std::cout << v.GetInt() << std::endl;

                        if (v.GetInt() != 0) {
                            IfcUtil::IfcBaseClass* ent = file.entityById(v.GetInt());

                            if (ent != NULL && ent->is(IfcSchema::IfcCartesianPoint::Class()) ) {
                                continue;
                            }
                        }

                        if (!first) {
                            s << ",";
                            // s << "WITH count(*) as dummy\n";
                            create_part << ",";
                        } else {
                            first = false;
                        }

                        has_data = true;
                        s << "(a" << counter << ":IfcNode{id:" << currentId << "}), (b" << counter << ":IfcNode{id:" << v.GetInt() << "})\n";

                        create_part << "(a" << counter << ")-[r" << counter << ":PropRel{name:'" << itr->name.GetString() << "'}]->(b" << counter << ")\n";


                        if (counter % 1 == 0 && counter != 0) {
                        // if (counter % 2 == 0 && counter != 0) {
                            if (neo_exec(*session, s.str() + create_part.str(), false) == false) {
                                return;
                            }

                            first = true;
                            s.str("");
                            create_part.str("");

                            s << "MATCH ";
                            create_part << "CREATE ";
                            has_data = false;
                        }

                        counter++;
                    }
                }
            }
        }

        // std::cout << s.str() << create_part.str() << std::endl;
        if (has_data && neo_exec(*session, s.str() + create_part.str())) {
            return;
        }

    }

    // END NEO

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

        s << "DROP TABLE IF EXISTS " << con.esc(importer::PROJECT_TABLE) << ";";
        W.exec( s.str().c_str() );
        s.str("");

        s << "DROP TYPE IF EXISTS " << con.esc(importer::DATA_IFCTYPE) << ";";
        W.exec( s.str().c_str() );
        s.str("");

        s << "CREATE TYPE " << con.esc(importer::DATA_IFCTYPE) << " AS ENUM (" << IfcTypes << ");";
        W.exec( s.str().c_str() );
        s.str("");

        s << "CREATE TABLE " << con.esc(importer::PROJECT_TABLE) << "("
          << "id             serial PRIMARY KEY,"
          << "name           TEXT UNIQUE NOT NULL)";
        W.exec( s.str().c_str() );
        s.str("");

        s << "CREATE TABLE " << con.esc(importer::DATA_TABLE) << "("
          << "id             serial PRIMARY KEY,"
          << "entityId       INT NOT NULL,"
          << "type           ifctypes    NOT NULL,"
          << "properties     JSONB     NOT NULL,"
          << "entity         TEXT,"
          << "geometry       JSONB,"
          << "projectid      integer NOT NULL REFERENCES " << con.esc(importer::PROJECT_TABLE) << " (id) )";
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


std::string get_filename (const std::string& str) {
    // std::cout << "Splitting: " << str << '\n';
  unsigned found = str.find_last_of("/\\");
  return str.substr(found + 1);
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

    std::string project = get_filename(std::string(argv[1]));

    try {
        pqxx::connection con("dbname=strusoft user=strusoft password=strusoft hostaddr=127.0.0.1 port=5432");

        if (con.is_open()) {
            std::cout << "Opened database successfully: " << con.dbname() << std::endl;
        } else {
            std::cout << "Can't open database" << std::endl;
            return 1;
        }

        // if (!create_table(con)) {
        //     std::cout << "Couldn't create table";
        //     return 1;
        // }

        int projectId = importer::doImport(file, con, project);

        // if (projectId < 0) {
        //     std::cout << "Couldn't create project.";
        //     return 1;
        // }

        importer::generateGeometry(file, con, projectId);
        // importer::generateGeometry(file, con, 0);

        // importer::build_neo_graph(file);

        con.disconnect();
        std::cout << "Connection closed." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
