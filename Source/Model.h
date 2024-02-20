//
// Created by Mark on 1/30/2024.
//

#pragma once

#include <string>
#include <optional>
#include <variant>
#include <map>
#include <vector>
#include <memory>
#include <tuple>
#include <iostream>
#include <sstream>
#include "JSONParser.h"

namespace ECE141 {
    const char singleQuote = '\'';
	class ModelQuery; // Forward declare
    enum class Comparable{GT,LT,GEQ,LEQ,EQ,NEQ};

	// STUDENT: Your Model is built from a bunch of these...
	struct ModelNode {
        ModelNode() : value{ModelNode::NullType{}} {}
        ModelNode(const ModelNode& anotherNode){
            this->operator=(anotherNode);
        }
        ModelNode& operator=(const ModelNode& anotherNode){
            this->value = anotherNode.value;
            return *this;
        }
        ModelNode(const std::string& initValue, const Element &aType, const ExactType &aExactType = ExactType::NullType){
            switch(aType){
                case Element::object:
                    value = ModelNode::ObjectType{}; break;
                case Element::array:
                    value = ModelNode::ListType{}; break;
                case Element::quoted:
                    value = initValue; break;
                case Element::closing: break;
                case Element::constant:
                    // could be null, bool, long, double
                    switch(aExactType){
                        case ExactType::BoolType:
                            if(initValue == "true"){value = true;}
                            else if(initValue == "false"){value = false;}
                            break;
                        case ExactType::LongType:
                            value = std::stol(initValue); break;
                        case ExactType::DoubleType:
                            value = std::stod(initValue); break;
                        case ExactType::NullType:
                            value = ModelNode::NullType{}; break;
                        default:
                            break;
                    }
                    break;
                case Element::unknown:
                    value = ModelNode::NullType{};
            }
        }

        // represent NULL using a tag
        struct NullType{};
        using ListType = std::vector<ModelNode>;
        using ObjectType = std::map<std::string, ModelNode>;

        std::variant<bool, long, double, std::string, ListType, ObjectType, NullType> value;
	};

    struct NodeVisitor {
        ExactType operator()(bool )       { return ExactType::BoolType; }
        ExactType operator()(double )   { return ExactType::DoubleType; }
        ExactType operator()(long )   { return ExactType::LongType; }
        ExactType operator()(std::string) { return ExactType::StringType; }
        ExactType operator()(ModelNode::ObjectType ) { return ExactType::ObjectType; }
        ExactType operator()(ModelNode::ListType ) { return ExactType::ListType; }
        ExactType operator()(ModelNode::NullType ) { return ExactType::NullType; }
    };


    class Model : public JSONListener {
	public:
		Model();
		~Model() override = default;
		Model(const Model& aModel);
		Model &operator=(const Model& aModel);

        // determines exact type of node to create, creates it, and stores in temp data member tempModelNode
        void createNode(const std::string& aValue, Element aType);
        // helper method to get exact type of Element::constant
        static ExactType getExactType(const std::string &aKey);
        ModelNode getRootNode();

		ModelQuery createQuery();

    protected:
		// JSONListener methods
		bool addKeyValuePair(const std::string &aKey, const std::string &aValue, Element aType) override;
		bool addItem(const std::string &aValue, Element aType) override;
		bool openContainer(const std::string &aKey, Element aType) override;
		bool closeContainer(const std::string &aKey, Element aType) override;

        ModelNode root; // default container to save model to once its built
        ModelNode tempModelNode; // only used when creating nodes during parsing as a convenience
        std::stack<ModelNode> nodeStack; // store all ModelNodes to determine which container to add new elems to
	};

    std::string toString(const ModelNode& aNode);
    // determines if container is array or object - needed for filter, get
    ExactType getContainerType(const ModelNode& aModelNode);
    // takes in a ModelNode of type Element::constant and outputs a string corresponding to the value in the Node
    std::string constNodeToString(const ModelNode& aNode, ExactType aType);
    std::tuple<int,int> getVecEndPts(std::string comparisonOperator, int ix);
    ModelNode createNullNode();

	class ModelQuery {
	public:
		explicit ModelQuery(Model& aModel);

		// ---Traversal---
		ModelQuery& select(const std::string& aQuery);

		// ---Filtering---
		ModelQuery& filter(const std::string& aQuery);

		// ---Consuming---
		size_t count();
		double sum();
		std::optional<std::string> get(const std::string& aKeyOrIndex);

	protected:
		Model &model;
        ModelNode tempNode;
	};


} // namespace ECE141