#include <iostream>
#include <fstream>
#include <picojson.h>
#include <tclap/CmdLine.h>

namespace gameplay
{
    struct Namespace
    {
        Namespace()
            : depth(0)
        {
        }

        Namespace(std::string const & name, int depth)
            : name(name)
            , depth(depth)
        {
        }

        int depth;
        std::string name;
        std::vector<std::pair<std::string, std::string>> values;
        std::vector<Namespace> namespaces;
    };

    std::string GetKeyName(picojson::value const & node)
    {
        std::string keyName;
        node.get(keyName);
        return keyName;
    }

    bool IsNamespaceType(picojson::value const & node)
    {
        return node.is<picojson::object>() || node.is<picojson::array>();
    }

    std::string GetIndentation(int depth)
    {
        static const int indentationSpaces = 4;
        return std::string(depth * indentationSpaces, ' ');
    }

    std::string GetFormattedNamespaceName(Namespace & namespaceToName, Namespace & parent)
    {
        std::string name = namespaceToName.name;

        if (name.empty())
        {
            name = parent.name + "_" + std::to_string(parent.namespaces.size());
        }

        return name;
    }

    void BeginNamespaceScope(Namespace & newNamespace, Namespace & parent, std::ofstream & stream)
    {
        stream << GetIndentation(newNamespace.depth) << GetFormattedNamespaceName(newNamespace, parent) << "\n";
        stream << GetIndentation(newNamespace.depth) << "{\n";
    }

    void EndNamespaceScope(Namespace & newNamespace, std::ofstream & stream)
    {
        stream << GetIndentation(newNamespace.depth) << "}\n\n";
    }

    void ConvertAndExport(picojson::value & currentNode, gameplay::Namespace & currentNamespace, std::ofstream & stream)
    {
        if (currentNode.is<picojson::array>())
        {
            int valueIndex = 0;

            for (auto & arrayValue : currentNode.get<picojson::array>())
            {
                if (IsNamespaceType(arrayValue))
                {
                    Namespace newNamespace(GetKeyName(arrayValue), currentNamespace.depth + 1);
                    BeginNamespaceScope(newNamespace, currentNamespace, stream);
                    ConvertAndExport(arrayValue, newNamespace, stream);
                    currentNamespace.namespaces.push_back(newNamespace);
                    EndNamespaceScope(newNamespace, stream);
                }
                else
                {
                    currentNamespace.values.emplace_back(std::to_string(valueIndex), "");
                    ConvertAndExport(arrayValue, currentNamespace, stream);
                    ++valueIndex;
                }
            }
        }
        else if (currentNode.is<picojson::object>())
        {
            for (auto & valuePair : currentNode.get<picojson::object>())
            {
                Namespace * nextNamespace = &currentNamespace;
                Namespace newNamespace;

                if (IsNamespaceType(valuePair.second))
                {
                    newNamespace.name = valuePair.first;
                    newNamespace.depth = currentNamespace.depth + 1;
                    nextNamespace = &newNamespace;
                    BeginNamespaceScope(newNamespace, currentNamespace, stream);
                }
                else
                {
                    nextNamespace->values.emplace_back(valuePair.first, "");
                }
                
                ConvertAndExport(valuePair.second, *nextNamespace, stream);

                if (nextNamespace == &newNamespace)
                {
                    currentNamespace.namespaces.push_back(newNamespace);
                    EndNamespaceScope(newNamespace, stream);
                }
            }
        }
        else
        {
            auto & valuePair = currentNamespace.values.back();
            valuePair.second = currentNode.to_str();
            stream << GetIndentation(currentNamespace.depth + 1) << valuePair.first << " = " << valuePair.second << "\n";
        }
    }
}

int main(int argc, char** argv)
{
    try
    {
        TCLAP::CmdLine cmd("JSON to Gameplay3D property converter");
        TCLAP::ValueArg<std::string> inputStreamArg("i", "input", "The JSON file to convert", true, "", "string", nullptr);
        TCLAP::ValueArg<std::string> outputFileArg("o", "output", "The Gameplay3D property file to output", true, "", "string", nullptr);
        cmd.add(inputStreamArg);
        cmd.add(outputFileArg);
        cmd.parse(argc, argv);
        
        std::ifstream inputStream;
        inputStream.open(inputStreamArg.getValue(), std::ios::in);
        
        std::string errors;

        if (inputStream)
        {
            picojson::value jsonDoc;
            errors = picojson::parse(jsonDoc, inputStream);

            if (errors.empty())
            {
                gameplay::Namespace rootNamespace("", -1);
                std::ofstream outputStream(outputFileArg.getValue());
                gameplay::ConvertAndExport(jsonDoc, rootNamespace, outputStream);
            }
        }
        else
        {
            errors = "Failed to open input file";
        }

        if (!errors.empty())
        {
            throw std::exception(errors.c_str());
        }
    }
    catch (std::exception ioException)
    {
        std::cerr << "error: " << ioException.what() << std::endl;
    }
    catch (TCLAP::ArgException & commandLineException)
    {
        std::cerr << "error: " << commandLineException.error() << " for arg " << commandLineException.argId() << std::endl;
    }
}