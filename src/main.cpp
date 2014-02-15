#include <iostream>
#include <fstream>
#include <picojson.h>
#include <tclap/CmdLine.h>

namespace converter
{
    typedef std::pair<std::string, std::string> ValuePair;

    class PropertyNamespace
    {
    public:
        enum class Modification
        {
            None,
            ValueAdded,
            NamespaceAdded
        };

        PropertyNamespace()
            : depth(0)
            , previousModification(Modification::None)
        {
        }

        PropertyNamespace(std::string const & name, int depth)
            : name(name)
            , depth(depth)
            , previousModification(Modification::None)
        {
        }

        void AddNamespace(PropertyNamespace & namespaceToAdd)
        {
            namespaces.push_back(namespaceToAdd);
            previousModification = Modification::NamespaceAdded;
        }

        void AddValue(std::string const name, std::string const value)
        {
            values.emplace_back(name, value);
            previousModification = Modification::ValueAdded;
        }

        ValuePair & GetLastValuePair()
        {
            return values.back();
        }

        size_t GetNamespaceCount() const
        {
            return namespaces.size();
        }

        Modification GetPreviousModification() const
        {
            return previousModification;
        }

        int depth;
        std::string name;
    private:
        Modification previousModification;
        std::vector<ValuePair> values;
        std::vector<PropertyNamespace> namespaces;
    };

    std::string ToString(int number)
    {
        std::ostringstream stringStream;
        stringStream << number;
        return stringStream.str();
    }

    bool IsNamespaceType(picojson::value const & node)
    {
        // Treat arrays and objects as namespaces since the property format has no equivalent concepts
        return node.is<picojson::object>() || node.is<picojson::array>();
    }

    std::string GetIndentation(int depth)
    {
        // Make indentation conform to that used in Gameplay3D samples
        static const int indentationSpaces = 4;
        return std::string(depth * indentationSpaces, ' ');
    }

    std::string GetFormattedNamespaceName(PropertyNamespace & namespaceToName, PropertyNamespace & parent)
    {
        std::string name = namespaceToName.name;

        // The property format doesn't allow un-named namespaces so create a name using the parents name and this
        // namespaces index into it
        if (name.empty())
        {
            name = parent.name + "_" + ToString(parent.GetNamespaceCount());
        }

        return name;
    }

    void BeginNamespaceScope(PropertyNamespace & newNamespace, PropertyNamespace & parent, std::ofstream & stream)
    {
        // Add vertical spacing between namespaces and values at the same depth
        if (parent.GetPreviousModification() != PropertyNamespace::Modification::None)
        {
            stream << std::endl;
        }

        std::string const indentation = GetIndentation(newNamespace.depth);
        stream << indentation << GetFormattedNamespaceName(newNamespace, parent) << std::endl;
        stream << indentation << "{\n";
    }

    void EndNamespaceScope(PropertyNamespace & newNamespace, std::ofstream & stream)
    {
        stream << GetIndentation(newNamespace.depth) << "}\n";
    }

    void ConvertAndExport(picojson::value & currentNode, converter::PropertyNamespace & currentNamespace, std::ofstream & stream)
    {
        if (currentNode.is<picojson::array>())
        {
            int valueIndex = 0;

            for (auto & arrayValue : currentNode.get<picojson::array>())
            {
                if (IsNamespaceType(arrayValue))
                {
                    // Create a nested namespace
                    std::string keyName;
                    arrayValue.get(keyName);
                    PropertyNamespace newNamespace(keyName, currentNamespace.depth + 1);
                    BeginNamespaceScope(newNamespace, currentNamespace, stream);
                    ConvertAndExport(arrayValue, newNamespace, stream);
                    currentNamespace.AddNamespace(newNamespace);
                    EndNamespaceScope(newNamespace, stream);
                }
                else
                {
                    // Array values are converted into key/value pairs within this namespace where
                    // they key is the values index into the array
                    currentNamespace.AddValue(ToString(valueIndex), "");
                    ConvertAndExport(arrayValue, currentNamespace, stream);
                    ++valueIndex;
                }
            }
        }
        else if (currentNode.is<picojson::object>())
        {
            // Same as arrays, creates nested namespaces and pairs, the pairs aren't indexed since
            // they already have names

            for (auto & valuePair : currentNode.get<picojson::object>())
            {
                PropertyNamespace * nextNamespace = &currentNamespace;
                PropertyNamespace newNamespace;

                if (IsNamespaceType(valuePair.second))
                {
                    newNamespace.name = valuePair.first;
                    newNamespace.depth = currentNamespace.depth + 1;
                    nextNamespace = &newNamespace;
                    BeginNamespaceScope(newNamespace, currentNamespace, stream);
                }
                else
                {
                    if (currentNamespace.GetPreviousModification() == PropertyNamespace::Modification::NamespaceAdded)
                    {
                        stream << std::endl;
                    }

                    nextNamespace->AddValue(valuePair.first, "");
                }
                
                ConvertAndExport(valuePair.second, *nextNamespace, stream);

                if (nextNamespace == &newNamespace)
                {
                    currentNamespace.AddNamespace(newNamespace);
                    EndNamespaceScope(newNamespace, stream);
                }
            }
        }
        else
        {
            // The entry for a value already exists in the current namespace and will always be the most recently
            // added so just fill in the value
            ValuePair & valuePair = currentNamespace.GetLastValuePair();
            valuePair.second = currentNode.to_str();

            // Add vertical spacing between namespaces and values at the same depth but allow sequential value
            // declarations to group together
            if (currentNamespace.GetPreviousModification() == PropertyNamespace::Modification::NamespaceAdded)
            {
                stream << std::endl;
            }

            stream << GetIndentation(currentNamespace.depth + 1) << valuePair.first << " = " << valuePair.second << std::endl;
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
        std::string errorMessage;

        if (inputStream)
        {
            picojson::value jsonDoc;
            std::cout << "Parsing JSON..." << std::endl;
            errorMessage = picojson::parse(jsonDoc, inputStream);

            if (errorMessage.empty())
            {
                std::ofstream outputStream(outputFileArg.getValue());

                if (outputStream)
                {
                    std::cout << "Converting..." << std::endl;
                    converter::PropertyNamespace rootNamespace("", -1);
                    converter::ConvertAndExport(jsonDoc, rootNamespace, outputStream);
                    std::cout << "Done" << std::endl;
                }
                else
                {
                    errorMessage = "Failed to create output stream";
                }
            }
        }
        else
        {
            errorMessage = "Failed to open input file";
        }

        if (!errorMessage.empty())
        {
            std::cerr << errorMessage.c_str() << std::endl;
        }
    }
    catch (TCLAP::ArgException & commandLineException)
    {
        std::cerr << commandLineException.error() << " for arg " << commandLineException.argId() << std::endl;
    }
}