Implementation notes:

- 5 helper functions in model.h - 1 factory method createNullNode()
- 1 extra visitor struct in model.h
- 2 extra enum classes - ConstType and Comparable
- Model class uses a stack to keep track of current open containers
- 

- Default constructor for ModelNode sets the value to NullType{}
- Added assignment operator to JSONState to allow assignment of currOpenContainer in Model
to the top of the states stack
- Model holds a stack of ModelNodes that is continuously updated during parsing
- Added enum class Exact to differentiate between all the types that ModelNode can hold
- Added method getConstType() to Model to find the specific constant type
the underlying ModelNode at any time
- Added helper method createNode that updates a data member tempNode in Model since its used
for openContainer, addKeyVal, and addItems
- Note that willParse in Parser.parse() takes care of the opening curly brace in any JSON file 
- ModelQuery holds a tempNode which is a selected and/or filtered ModelNode object to pass into
sum/get/count
- getContainerType() uses the visitor pattern with std::visit to determine the object type
- Assumption: when writing get(), helper fcn toString() and constNodeToString()
assumes that if a map is printed, it only has key value pairs and no nested objects
- Error handling for sum - checks to see if ModelQuery::tempNode is ListNode type - if not,
throws std::invalid_argument
  - Also checks to ensure all elements are numerics - throws bad variant access error if not
- Error handling for constNodeToString - throws std::invalid_argument if list/object type passed in
