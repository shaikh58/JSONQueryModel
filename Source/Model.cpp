//
// Created by Mark on 1/30/2024.
//

#include "Model.h"
#include "Debug.h"
#include "Formatting.h"

namespace ECE141 {

	// ---Model---

	Model::Model() = default;

	Model::Model(const Model& aModel) {
		this->operator=(aModel);
	}

	Model &Model::operator=(const Model& aModel) {
        this->root = aModel.root; // simply copy over the "root" map that the entire json is stored in
		return *this;
	}

	ModelQuery Model::createQuery() {
		return ModelQuery(*this);
	}

    ExactType Model::getExactType(const std::string &aVal){
        // for null, bool, long, double
        if(aVal == "null"){return ExactType::NullType;}
        else if(aVal == "true" || aVal == "false"){return ExactType::BoolType;}
        else if(aVal.find('.') != std::string::npos){return ExactType::DoubleType;}
        else{return ExactType::LongType;}
    }

    void Model::createNode(const std::string& aValue, Element aType){
        ExactType theExactType = (aType == Element::constant) ? getExactType(aValue) : ExactType::NullType;
        this->tempModelNode = ModelNode(aValue, aType, theExactType);
    }

	bool Model::addKeyValuePair(const std::string& aKey, const std::string& aValue, Element aType) {
        createNode(aValue, aType); // this fcn updates member var tempModelNode which is used below
        //  we know the ModelNode variant must be an object type if we're in this fcn, so get a reference to it and insert
        auto& stackTop = nodeStack.top();
        std::get<ModelNode::ObjectType>(stackTop.value).insert({aKey, tempModelNode});\
		DBG("\t'" << aKey << "' : '" << aValue << "'");
		return true;
	}

	bool Model::addItem(const std::string& aValue, Element aType) {
        createNode(aValue, aType); // this fcn updates member var tempModelNode which is used below
        auto& stackTop = nodeStack.top();
        std::get<ModelNode::ListType>(stackTop.value).push_back(tempModelNode);
		DBG("\t'" << aValue << "'");
		return true;
	}

	bool Model::openContainer(const std::string& aContainerName, Element aType) {
        createNode(aContainerName, aType);
        nodeStack.push(tempModelNode); // can only be either a map or vector

		DBG((aContainerName.empty() ? "EMPTY" : aContainerName) << " " << (aType == Element::object ? "{" : "["));
		return true;
	}

	bool Model::closeContainer(const std::string& aContainerName, Element aType) {
        // top() is a reference so omit the & in auto to create a copy so that when popped, the node still exists
        auto currContainerCopy = nodeStack.top();
        // now that a copy is made of curr container, remove the original one from the stack
        nodeStack.pop();
        // at the end, when root node is popped, store this in member variable root and exit the function
        if(nodeStack.size() == 0){
            root = currContainerCopy;
            return true;
        }
        ExactType prevNodeType = getContainerType(nodeStack.top());
        // now insert it into its parents container
        if(prevNodeType == ExactType::ObjectType){
            std::get<ModelNode::ObjectType>(nodeStack.top().value).insert({aContainerName, currContainerCopy});
        }
        else if(prevNodeType == ExactType::ListType){
            std::get<ModelNode::ListType>(nodeStack.top().value).push_back(currContainerCopy);
        }

		DBG(" " << (aType == Element::object ? "}" : "]"));
		return true;
	}

    ModelNode Model::getRootNode() {
        return root;
    }

	// ---ModelQuery---

    ModelNode createNullNode(){
        return ModelNode("",Element::unknown, ExactType::NullType);
    }

    ModelQuery::ModelQuery(Model &aModel) : model(aModel) {}

	ModelQuery& ModelQuery::select(const std::string& aQuery) {
        std::istringstream aQueryStream(aQuery); // create a stream from the input query string
        std::string aKey;
        int ix;
        auto currNode = this->model.getRootNode(); // copy of the model root node
        ModelNode::ObjectType objectNode;
        ModelNode::ListType listNode;

        while(std::getline(aQueryStream, aKey, '.')){ // get each word from query

            if(aKey[0] == singleQuote){ // case where it's a key for a map
                aKey = aKey.substr(1,aKey.length() - 2); // get the actual key between the single quotes
                objectNode = std::get<ModelNode::ObjectType>(currNode.value); // get currNode's map
                auto it = objectNode.find(aKey);
                if(it != objectNode.end()){
                    currNode = it->second; // currNode is effectively a "pointer" that moves along the root map
                }
                else{
                    // if key not found, return a nulltype ModelNode
                    this->tempNode = createNullNode();
                    return *this;
                }
            }
            else{ // case where it's a list index
                ix = std::stoi(aKey);
                listNode = std::get<ModelNode::ListType>(currNode.value); // get currNode's vector
                try{
                    currNode = listNode[ix];
                }
                catch(std::out_of_range &e){
                    this->tempNode = createNullNode();
                    return *this;
                }
            }
        }
        this->tempNode = currNode;

		DBG("select(" << aQuery << ")");
		return *this;
	}

    ExactType getContainerType(const ModelNode& aModelNode){
        ExactType theType = std::visit(NodeVisitor(), aModelNode.value);
        return theType;
    }

    std::string constNodeToString(const ModelNode& aNode, ExactType aType){
        switch(aType){
            case ExactType::StringType:
                return '"' + std::get<std::string>(aNode.value) + '"';
            case ExactType::DoubleType:
                return doubleToString(std::get<double>(aNode.value));
            case ExactType::LongType:
                return std::to_string(std::get<long>(aNode.value));
            case ExactType::BoolType:
                return (std::to_string(std::get<bool>(aNode.value)) == "1") ? "true" : "false";
            case ExactType::NullType:
                return std::string{"null"};
            default:
                throw std::invalid_argument("Invalid value type encountered in map; "
                                            "only accepts string, double, long, bool");
        }
    }

    std::string toString(const ModelNode& aNode){
        // takes in any ModelNode and returns a string of the top level container's items (nested containers not supported)
        std::ostringstream outStream;
        auto theMap = ModelNode::ObjectType{};
        auto theList = ModelNode::ListType{};
        ExactType nodeType = getContainerType(aNode);
        switch(nodeType){
            case ExactType::ObjectType:
                theMap = std::get<ModelNode::ObjectType>(aNode.value);
                outStream << "{";
                for(auto it = theMap.begin(); it != theMap.end(); ++it){
                    outStream << '"' << it->first << '"' << ":";
                    // value can only be bool, long, double, string, null - all others throw exception
                    outStream << constNodeToString(it->second, getContainerType(it->second));
                    if(std::next(it) == theMap.end()){break;}
                    else{outStream << ",";}
                }
                outStream << "}";
                break;
            case ExactType::ListType: // assume the list only has Element::constant types in it i.e. no nested objects or lists
                theList = std::get<ModelNode::ListType>(aNode.value);
                outStream << "[";
                for(auto& element:theList){
                    outStream << constNodeToString(element, getContainerType(element)) << ",";
                }
                break;
            default:
                outStream << constNodeToString(aNode, nodeType);
        }

        return outStream.str();
    }

    std::tuple<int,int> getVecEndPts(std::string comparisonOperator, int ix){
        int start = 0; int end = 0;
        std::map<std::string, Comparable> comparablesMap = {
                {"==", Comparable::EQ}, {"!=", Comparable::NEQ},
                {"> ", Comparable::GT}, {"< ", Comparable::LT},
                {">=", Comparable::GEQ}, {"<=", Comparable::LEQ}
        };
        Comparable theComparable = comparablesMap[comparisonOperator];
        switch(theComparable){
            case Comparable::EQ:
                start = ix; end = ix; break;
            case Comparable::NEQ:
                start = -1; end = -1; break;
            case Comparable::GT:
                start = ix + 1; end = -1; break;
            case Comparable::LT:
                start = 0; end = ix - 1; break;
            case Comparable::GEQ:
                start = ix; end = -1; break;
            case Comparable::LEQ:
                start = 0; end = ix; break;
        }
        return std::make_tuple(start,end);
    }

	ModelQuery& ModelQuery::filter(const std::string& aQuery) {
        //  only called with maps or lists
        auto currNode = this->tempNode; // tempNode updated after select() - make copy for now, then update it
        ExactType nodeType = getContainerType(currNode); // tempNode updated after select()
        auto theFiltNodeMap = ModelNode("", Element::object);
        auto theFiltNodeVec = ModelNode("", Element::array);

        if(aQuery.substr(0,3) == "key"){
            if(nodeType != ExactType::ObjectType){
                throw std::invalid_argument("Attempting to filter a non-map node using a key");
            }
            auto theMap = std::get<ModelNode::ObjectType>(this->tempNode.value);
            // insert each filtered key-val pair from theMap into theFiltNodeMapVal
            auto& theFiltNodeMapVal = std::get<ModelNode::ObjectType>(theFiltNodeMap.value);

            std::string queryStr = aQuery.substr(13); // gets the substring to be found in the keys
            queryStr = queryStr.substr(1,queryStr.length() - 2);
            for(auto& element: theMap){
                if(element.first.find(queryStr) != std::string::npos){
                    theFiltNodeMapVal[element.first] = element.second;
                }
            }
            this->tempNode = theFiltNodeMap;
        }
        // case where aQuery has an index comparison
        else{
            auto theVec = std::get<ModelNode::ListType>(this->tempNode.value);
            auto& theFiltNodeVecVal = std::get<ModelNode::ListType>(theFiltNodeVec.value); // reference to node

            bool skip_ix = false;
            std::string comparisonOp = aQuery.substr(6,2); // extracts the comparison operator
            int comparisonVal = std::stoi(aQuery.substr(aQuery.length() - 2)); // extracts single/double digit indexes

            // this block gets the start and end indexes that we want to extract from the original node vector
            auto endPts = getVecEndPts(comparisonOp, comparisonVal);
            int start = std::get<0>(endPts); int end = std::get<1>(endPts);
            // if the index is larger than the container its looking in, return an empty ModelNode
            if(end >= static_cast<int>(theVec.size())) {
                this->tempNode = createNullNode();
                return *this;
            }
            // case when it's a not equals operator
            if(start == -1){
                skip_ix = true;
                start = 0;
            }
            if(end == -1){ end = theVec.size() - 1; }
            for (int i = start; i <= end; i++){
                if(skip_ix && i==comparisonVal){}
                else{ theFiltNodeVecVal.push_back(theVec[i]); }
            }

            this->tempNode = theFiltNodeVec;
        }

		DBG("filter(" << aQuery << ")");
		return *this;
	}

	size_t ModelQuery::count() {
        // assumes that tempNode has been set by select() and/or filter(), and can only be ObjectType or ListType
        ExactType theType = getContainerType(tempNode);
        auto theMap = ModelNode::ObjectType{};
        auto theList = ModelNode::ListType{};
        switch(theType){
            case ExactType::ObjectType:
                theMap = std::get<ModelNode::ObjectType>(tempNode.value);
                return theMap.size();
            case ExactType::ListType:
                theList = std::get<ModelNode::ListType>(tempNode.value);
                return theList.size();
            default:
                break;
        }
		DBG("count()");
        return static_cast<size_t>(0);
	}

	double ModelQuery::sum() {
        // operates on a ModelNode which can only contain ListNode stored in this->tempNode after select() and/or filter
        // error checking - if tempNode is not a list, throw an exception
        ExactType theType = getContainerType(tempNode);
        if(theType != ExactType::ListType){ throw std::invalid_argument("Attempting to sum over a non-list container"); }
        double sum = 0.0;
        double val;
        auto theList = std::get<ModelNode::ListType>(tempNode.value);
        for(auto& element: theList){
            // only long and double types in theList - if this conversion fails, there is an invalid input
            ExactType elementType = getContainerType(element);
            switch(elementType){
                case ExactType::LongType:
                    val = std::get<long>(element.value);
                    break;
                case ExactType::DoubleType:
                    val = std::get<double>(element.value);
                    break;
                default:
                    throw std::invalid_argument("Attempting to sum a non-numeric value");
            }
            sum += val;
        }
		DBG("sum()");
		return sum;
	}

	std::optional<std::string> ModelQuery::get(const std::string& aKeyOrIndex) {
        // input can only be an index or key so this method can only be called on object or list types
        bool anIndex = true;
        int ix;
        std::string aKey;
        ModelNode outNode;
        ExactType nodeType = getContainerType(tempNode); // tempNode updated after select()
        if(nodeType == ExactType::NullType){ return std::nullopt; } // if invalid key/index was given to select,
        // get() will receive a NullType ModelNode

        try{ ix = std::stoi(aKeyOrIndex); }
        catch(std::invalid_argument &e){ anIndex = false; }

        if(anIndex){
            if(nodeType == ExactType::ListType){
                // error handling for case when index is out of bounds of list
                auto& theVar = std::get<ModelNode::ListType>(tempNode.value);
                if(ix >= static_cast<int>(theVar.size())){return std::nullopt;}
                else{
                    outNode = theVar[ix];
                }
                return toString(outNode); // can be any ModelNode type
            }
            else{ return std::nullopt; }
        }
        else{
            // input arg is a key
            if(aKeyOrIndex == "*"){
                return toString(tempNode);
            }
            aKey = aKeyOrIndex.substr(1, aKeyOrIndex.length() - 2);
            // error handling for invalid key
            try{
                outNode = std::get<ModelNode::ObjectType>(tempNode.value)[aKey];
            }
            catch(std::out_of_range &e) {
                return std::nullopt;
            }
            return toString(outNode); // can be any ModelNode type
        }

		DBG("get(" << aKeyOrIndex << ")");
        return std::nullopt;
	}

} // namespace ECE141