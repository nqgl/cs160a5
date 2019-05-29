#include "typecheck.hpp"

// Defines the function used to throw type errors. The possible
// type errors are defined as an enumeration in the header file.
void typeError(TypeErrorCode code) {
  switch (code) {
    case undefined_variable:
      std::cerr << "Undefined variable." << std::endl;
      break;
    case undefined_method:
      std::cerr << "Method does not exist." << std::endl;
      break;
    case undefined_class:
      std::cerr << "Class does not exist." << std::endl;
      break;
    case undefined_member:
      std::cerr << "Class member does not exist." << std::endl;
      break;
    case not_object:
      std::cerr << "Variable is not an object." << std::endl;
      break;
    case expression_type_mismatch:
      std::cerr << "Expression types do not match." << std::endl;
      break;
    case argument_number_mismatch:
      std::cerr << "Method called with incorrect number of arguments." << std::endl;
      break;
    case argument_type_mismatch:
      std::cerr << "Method called with argument of incorrect type." << std::endl;
      break;
    case while_predicate_type_mismatch:
      std::cerr << "Predicate of while loop is not boolean." << std::endl;
      break;
    case do_while_predicate_type_mismatch:
      std::cerr << "Predicate of do while loop is not boolean." << std::endl;
      break;
    case if_predicate_type_mismatch:
      std::cerr << "Predicate of if statement is not boolean." << std::endl;
      break;
    case assignment_type_mismatch:
      std::cerr << "Left and right hand sides of assignment types mismatch." << std::endl;
      break;
    case return_type_mismatch:
      std::cerr << "Return statement type does not match declared return type." << std::endl;
      break;
    case constructor_returns_type:
      std::cerr << "Class constructor returns a value." << std::endl;
      break;
    case no_main_class:
      std::cerr << "The \"Main\" class was not found." << std::endl;
      break;
    case main_class_members_present:
      std::cerr << "The \"Main\" class has members." << std::endl;
      break;
    case no_main_method:
      std::cerr << "The \"Main\" class does not have a \"main\" method." << std::endl;
      break;
    case main_method_incorrect_signature:
      std::cerr << "The \"main\" method of the \"Main\" class has an incorrect signature." << std::endl;
      break;
  }
  exit(1);
}

// HELPER functions:



CompoundType getExpressionType(ExpressionNode* expression, Visitor* visitor){
    if (typeid(*expression) == typeid(PlusNode)
      || (typeid(*expression) == typeid(MinusNode))
      || (typeid(*expression) == typeid(TimesNode))
      || (typeid(*expression) == typeid(DivideNode))
      || (typeid(*expression) == typeid(NegationNode))
      || (typeid(*expression) == typeid(IntegerLiteralNode))){
      CompoundType intType = {bt_integer, ""};
      return intType;
    } else if (typeid(*expression) == typeid(GreaterNode)
      || (typeid(*expression) == typeid(GreaterEqualNode))
      || (typeid(*expression) == typeid(EqualNode))
      || (typeid(*expression) == typeid(AndNode))
      || (typeid(*expression) == typeid(OrNode))
      || (typeid(*expression) == typeid(NotNode))
      || (typeid(*expression) == typeid(BooleanLiteralNode))){
      CompoundType booleanType = {bt_boolean, ""};
      return booleanType;
    }
    else if (typeid(*expression) == typeid(VariableNode)){
      VariableInfo& varInfo;
      varInfo = getVariableInfo(expression->identifier, visitor);
      return varInfo->type;
    }
    else if (typeid(*expression) == typeid(MethodCallNode) {
      std::string callertype;
      if (expression->identifier_1 == "" || expression->identifier_1 == NULL){
        // if first id doesn't exist
        callertype = visitor->currentClassName;
      }
      else {
        VariableInfo objectInfo = getVariableInfo(expression->identifier_1);
        if (objectInfo.type.baseType != bt_object){
          // we would be unable to access a method of it, thus error in source code
          typeError(not_object);
        }
        callertype = objectInfo.type.objectClassName;
      }
      MethodInfo& methodCalled = getMethodInfoFromClass(callertype, expression->identifier_2, visitor);
      if (methodCalled->returnType->baseType == bt_none && methodCalled->returnType->objectClassName == "failure"){
        typeError(undefined_method); 
      }
      else {
        return methodCalled->returnType;
      }
    }
    else if (typeid(*expression) == typeid(MemberAccessNode))) {
      VariableInfo objectInfo = getVariableInfo(expression->identifier_1);
      if (objectInfo.type.baseType != bt_object){
        // we would be unable to access a member of it, thus error in source code
        typeError(not_object);
      }
      VariableInfo& checkInfo = getVariableInfoFromClassMember(objectInfo.type.objectClassName, expression->identifier_2, visitor);
      if (checkInfo->type->baseType == bt_none && checkInfo->type->objectClassName == "failure"){
        typeError(undefined_member); 
      }
      else {
        return checkInfo->type;
      }
    }
    else if (typeid(*expression) == typeid(NewNode)) {
      // todo
      // assuming that there always must be a constructor defined
      if (visitor->currentClassTable.count(expression->identifier) == 0){
        typeError(undefined_class);
      }
      ClassInfo typeClassInfo& = visitor->currentClassTable.at(expression->identifier);
      if (typeClassInfo->methods->count(expression->identifier) == 0){
        typeError(undefined_method);
      }
      else{
        MethodInfo constructorInfo = typeClassInfo->methods->at(expression->identifier);
        // go thru expression list
        std::list<ExpressionNode*>::iterator i_args = expression->expression_list->begin();
        std::list<CompoundType>::iterator i_params = constructorInfo->parameters->begin();
        CompoundType& argType;
        while(i_params != constructorInfo->parameters->end() && i_args != expression->expression_list->end()){
          argType = getExpressionType(*i_args, visitor);
          if ((argType->baseType != i_params->baseType) 
          || (argType->objectClassName != i_params->objectClassName)){
            typeError(argument_type_mismatch);
          }
          i_args++; 
          i_params++;
        }
        if (i_args != constructorInfo->parameters->end() || i_params != expression->expression_list->end()){
          typeError(argument_number_mismatch);
        }
      }
    }
}



// maybe a function to look up a name in current scope
// unclear whether it should be a different function for looking up a methodinfo vs a vaiableinfo vs a classinfo

ClassInfo getClassInfo(std::string& identifier, Visitor* scope) {
    if (scope->classTable->count(identifier) != 0){
        return (*(scope->classTable))[identifier];
    }
    else {
        typeError(undefined_class);
    }
}

VariableInfo getVariableInfo(std::string& identifier, Visitor* scope){
    // looks for, in this order:
    // local variables
    // parameters (how to find their name? not in methodinfo)
    // member of class (or superclass (check recursively probably))
    // try search in currentvariabletable
    if (scope->currentVariableTable->count(identifier) != 0){
        // searches both locals and parameters, 
        // because both are stored in the same variable table
        return scope->currentVariableTable->at(identifier);    
    }
    else {
      VariableInfo& checkInfo = getVariableInfoFromClassMember(scope->currentClassName, identifier, scope);
      if (checkInfo->type->baseType == bt_none && checkInfo->type->objectClassName == "failure"){
        typeError(undefined_variable); 
      }
    }
}

VariableInfo getVariableInfoFromClassMember(std::string& classname, std::string& identifier, Visitor* scope){
    ClassInfo& classInfo = getClassInfo(classname, scope);
    if (classInfo->members->count(identifier) != 0) {
        return classInfo->members->at(identifier);
    }
    else if (classInfo.superClassName != ""){
        if (scope->classTable->count(classInfo.superClassName) != 0){
            return getVariableInfoFromClassMember(classInfo.superClassName, identifier, scope);
        }
        else {
            CompoundType failuretype {bt_none, "failure"};
            VariableInfo varinfo {failuretype, -1, -1};
            return varinfo
        }        
    }
}


MethodInfo getMethodInfoFromClass(std::string& classname, std::string& identifier, Visitor* scope){
   ClassInfo& classInfo = getClassInfo(classname, scope);
    if (classInfo->methods->count(identifier) != 0) {
        return classInfo->methods->at(identifier);
    }
    else if (classInfo.superClassName != ""){
        if (scope->classTable->count(classInfo.superClassName) != 0){
            return getMethodInfoFromClass(classInfo.superClassName, identifier, scope);
        }
        else {
            CompoundType failuretype {bt_none, "failure"};
            VariableInfo varinfo {failuretype, -1, -1};
            return varinfo
        }        
    } 
}


// TypeCheck Visitor Functions: These are the functions you will
// complete to build the symbol table and type check the program.
// Not all functions must have code, many may be left empty.

void TypeCheck::visitProgramNode(ProgramNode* node) {
  // WRITEME: Replace with code if necessary
  this->classTable = new ClassTable();

  node->visit_children(this);

  if (classTable->count("Main")==0){
    TypeErrorCode(no_main_class);
  }
  ClassInfo main_class = classTable['Main'];
  if (main_class.methods->count("main")==0) {
    TypeErrorCode(no_main_method);
  }
  if (!main_class.members->empty()){
    TypeErrorCode(main_class_members_present);
  }
  MethodInfo main_method = methodTable["main"];
  if (!main_method.returnType == "none" || main_method.parameters->size() != 0){
    TypeErrorCode(main_method_incorrect_signature);
  }
}


void TypeCheck::visitClassNode(ClassNode* node) {
  // WRITEME: Replace with code if necessary
  ClassInfo classInfo;
  classInfo.methods = new MethodTable();
  classInfo.members = new VariableTable();
  classInfo.superClassName = node->identifier_2->name; // probably need to 
  this->currentMethodTable = classInfo.methods;
  this->currentVariableTable = classInfo.members;

  this->currentClassName = node ->identifier_1->name;
  node->visit_children(this);
  classInfo.membersSize = classInfo.members->size()*4; //TA magic

  (*(this->classTable))[identifier_1] = classInfo;

  if (this->classTable->count(currentClassName)==0){ // if constructor returns a type, throw error
    TypeErrorCode(undefined_class);
  }


}

void TypeCheck::visitMethodNode(MethodNode* node) {
  // WRITEME: Replace with code if necessary
  MethodInfo method;
  /*
  method.returnType = node->methodbody->returnstatement->expression->baseType;
  method.parameters = node->parameter_list;
  method.variables = node->methodbody->decleration_list;
  currentVariableTable =method.variables;
*/
  
  /* method.returnType = node->type->basetype;
  method.parameters = node->parameter_list;
  currentVariableTable =method.variables;
*/

  node->visit_children(this);

  method.localsSize = method.variables->size()*4;
  (*(this->methodTable))[identifier]= method;

  if (method.nodetype != )

  // check if method is constructor
  // by if it has the same name as current class return statement is type of method
  if (node->identifier == this->currentClassName){
    if ((*(this->methodTable))[node->identifier]->returnType->baseType != bt_none){
      typeError(constructor_returns_type);
    }
  }

  // check return type
  ExpressionNode* returnExpresion = node->methodbody->returnstatement->expression;
  switch ()

  (*(this->methodTable))[node->identifier]->returnType->baseType == node->methodbody->returnstatement->baseType
  if ((*(this->methodTable))[node->identifier]->returnType->baseType != bt_none){
  }

  (*(this->methodTable))[]
}
}
if (node->baseType != method.returnType){
  TypeErrorCode(return_type_mismatch);
}
} //todo fixme

void TypeCheck::visitMethodBodyNode(MethodBodyNode* node) {
  // WRITEME: Replace with code if necessary
  // do we want to pass in the next methodbodyinfo by 

  node->basetype = node->returnstatement->basetype;
  node->basetype = bt_object; // I don't think we want to do this assignment
                              // eg, what if it's a bt_integer instead

  node->objectClassName = node->identifier->name;
  currentVariableTable = method.variables



  node->baseType = node->returnstatement->baseType;


}

void TypeCheck::visitParameterNode(ParameterNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitDeclarationNode(DeclarationNode* node) {
  // WRITEME: Replace with code if necessary

  // increase offset count  
}

void TypeCheck::visitReturnStatementNode(ReturnStatementNode* node) {
  // WRITEME: Replace with code if necessary // WRITEME: Replace with code if necessary
  
  // 
  //
}

void TypeCheck::visitAssignmentNode(AssignmentNode* node) {
  // WRITEME: Replace with code if necessary  
}

void TypeCheck::visitCallNode(CallNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitIfElseNode(IfElseNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitWhileNode(WhileNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitDoWhileNode(DoWhileNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitPrintNode(PrintNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitPlusNode(PlusNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitMinusNode(MinusNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitTimesNode(TimesNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitDivideNode(DivideNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitGreaterNode(GreaterNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitGreaterEqualNode(GreaterEqualNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitEqualNode(EqualNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitAndNode(AndNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitOrNode(OrNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitNotNode(NotNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitNegationNode(NegationNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitMethodCallNode(MethodCallNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitMemberAccessNode(MemberAccessNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitVariableNode(VariableNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitIntegerLiteralNode(IntegerLiteralNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitBooleanLiteralNode(BooleanLiteralNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitNewNode(NewNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitIntegerTypeNode(IntegerTypeNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitBooleanTypeNode(BooleanTypeNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitObjectTypeNode(ObjectTypeNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitNoneNode(NoneNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitIdentifierNode(IdentifierNode* node) {
  // WRITEME: Replace with code if necessary
}

void TypeCheck::visitIntegerNode(IntegerNode* node) {
  // WRITEME: Replace with code if necessary
}


// The following functions are used to print the Symbol Table.
// They do not need to be modified at all.

std::string genIndent(int indent) {
  std::string string = std::string("");
  for (int i = 0; i < indent; i++)
    string += std::string(" ");
  return string;
}

std::string string(CompoundType type) {
  switch (type.baseType) {
    case bt_integer:
      return std::string("Integer");
    case bt_boolean:
      return std::string("Boolean");
    case bt_none:
      return std::string("None");
    case bt_object:
      return std::string("Object(") + type.objectClassName + std::string(")");
    default:
      return std::string("");
  }
}


void print(VariableTable variableTable, int indent) {
  std::cout << genIndent(indent) << "VariableTable {";
  if (variableTable.size() == 0) {
    std::cout << "}";
    return;
  }
  std::cout << std::endl;
  for (VariableTable::iterator it = variableTable.begin(); it != variableTable.end(); it++) {
    std::cout << genIndent(indent + 2) << it->first << " -> {" << string(it->second.type);
    std::cout << ", " << it->second.offset << ", " << it->second.size << "}";
    if (it != --variableTable.end())
      std::cout << ",";
    std::cout << std::endl;
  }
  std::cout << genIndent(indent) << "}";
}

void print(MethodTable methodTable, int indent) {
  std::cout << genIndent(indent) << "MethodTable {";
  if (methodTable.size() == 0) {
    std::cout << "}";
    return;
  }
  std::cout << std::endl;
  for (MethodTable::iterator it = methodTable.begin(); it != methodTable.end(); it++) {
    std::cout << genIndent(indent + 2) << it->first << " -> {" << std::endl;
    std::cout << genIndent(indent + 4) << string(it->second.returnType) << "," << std::endl;
    std::cout << genIndent(indent + 4) << it->second.localsSize << "," << std::endl;
    print(*it->second.variables, indent + 4);
    std::cout <<std::endl;
    std::cout << genIndent(indent + 2) << "}";
    if (it != --methodTable.end())
      std::cout << ",";
    std::cout << std::endl;
  }
  std::cout << genIndent(indent) << "}";
}

void print(ClassTable classTable, int indent) {
  std::cout << genIndent(indent) << "ClassTable {" << std::endl;
  for (ClassTable::iterator it = classTable.begin(); it != classTable.end(); it++) {
    std::cout << genIndent(indent + 2) << it->first << " -> {" << std::endl;
    if (it->second.superClassName != "")
      std::cout << genIndent(indent + 4) << it->second.superClassName << "," << std::endl;
    print(*it->second.members, indent + 4);
    std::cout << "," << std::endl;
    print(*it->second.methods, indent + 4);
    std::cout <<std::endl;
    std::cout << genIndent(indent + 2) << "}";
    if (it != --classTable.end())
      std::cout << ",";
    std::cout << std::endl;
  }
  std::cout << genIndent(indent) << "}" << std::endl;
}

void print(ClassTable classTable) {
  print(classTable, 0);
}
