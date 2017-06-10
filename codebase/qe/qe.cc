
#include "qe.h"
#include <string>
#include <cmath>
#include <cstring>

Filter::Filter(Iterator* input, const Condition &condition) {
    filterInput = input;
    filterCondition = condition;
}

RC Filter::getNextTuple(void *data){    
    vector<Attribute> attrs;
    filterInput->getAttributes(attrs);
    void* iteratorBuffer = malloc(1000);
    // filterInput->getNextTuple(iteratorBuffer);

    // string leftTable;
    // string leftAttribute;
    // splitLeftAttribute(filterCondition, leftTable, leftAttribute);
    string leftAttribute = filterCondition.lhsAttr;
    while(filterInput->getNextTuple(iteratorBuffer) != QE_EOF){
cout<<*((int*)((char*)iteratorBuffer+1))<<endl;
        bool validTuple;
        int i;
        int offset = 1;
        for(i=0; i<attrs.size(); i++){
// cout<<"leftAttribute: "<<leftAttribute<<endl;
// cout<<"attrs.name: "<<attrs[i].name<<endl;
            if(leftAttribute == attrs[i].name){
                break;
            }
            if(attrs[i].type == TypeInt){
                offset += sizeof(int);
            }else if(attrs[i].type == TypeReal){
                offset+= sizeof(float);
            }else{
                int sizeOfVarCharInRecord;
                memcpy(&sizeOfVarCharInRecord, (char*)iteratorBuffer + offset, sizeof(int));
                offset+= sizeof(int) + sizeOfVarCharInRecord;
            }
        }
        if(filterCondition.bRhsIsAttr == false){
            if(filterCondition.rhsValue.type==TypeInt){
                int intToCompare;
                memcpy(&intToCompare, (char*)iteratorBuffer + offset, sizeof(int));
                validTuple = checkScanCondition(intToCompare, filterCondition);
                if(validTuple == true){
                    memcpy((char*)data, (char*)iteratorBuffer, 200);
                    return SUCCESS;
                }
            }else if(filterCondition.rhsValue.type==TypeReal){
                float realToCompare;
                memcpy(&realToCompare, (char*)iteratorBuffer + offset, sizeof(float));
                validTuple = checkScanCondition(realToCompare, filterCondition);
                if(validTuple == true){
                    memcpy((char*)data, (char*)iteratorBuffer,200);
                    return SUCCESS;
                }
            }else{
                int sizeOfVarChar;
                memcpy(&sizeOfVarChar, (char*)iteratorBuffer + offset, sizeof(int));
                char varCharBuffer[sizeOfVarChar+1];
                memcpy(varCharBuffer, (char*)iteratorBuffer + sizeof(int) + offset, sizeOfVarChar);
                varCharBuffer[sizeOfVarChar] = '\0';
                validTuple = checkScanCondition(varCharBuffer, filterCondition);
cout<<"valid Tuple: "<<validTuple<<endl;
                if(validTuple == true){
                    memcpy((char*)data, (char*)iteratorBuffer,200);
                    return SUCCESS;
                }
            }
            memset(iteratorBuffer, 0, 1000);
        }else{
            // string rightTable;
            // string rightAttribute;
            // splitRightAttribute(filterCondition, rightTable, rightAttribute);
            // return QE_EOF;
            memcpy((char*)data, (char*)iteratorBuffer,200);
            return SUCCESS;
        }
    }
    return QE_EOF;  
    
}

void Filter::getAttributes(vector<Attribute> &attrs) const{
    filterInput->getAttributes(attrs);
}

bool Filter::checkScanCondition(int intToCompare, Condition &condition){
    switch(condition.op){
        case EQ_OP: return intToCompare == *(int*)condition.rhsValue.data;
        case LT_OP: return intToCompare < *(int*)condition.rhsValue.data;
        case GT_OP: return intToCompare > *(int*)condition.rhsValue.data;
        case LE_OP: return intToCompare <= *(int*)condition.rhsValue.data;
        case GE_OP: return intToCompare >= *(int*)condition.rhsValue.data;
        case NE_OP: return intToCompare != *(int*)condition.rhsValue.data;
        case NO_OP: return true;
    }
}

bool Filter::checkScanCondition(float realToCompare, Condition &condition){
    switch(condition.op){
        case EQ_OP: return realToCompare == *(float*)condition.rhsValue.data;
        case LT_OP: return realToCompare < *(float*)condition.rhsValue.data;
        case GT_OP: return realToCompare > *(float*)condition.rhsValue.data;
        case LE_OP: return realToCompare <= *(float*)condition.rhsValue.data;
        case GE_OP: return realToCompare >= *(float*)condition.rhsValue.data;
        case NE_OP: return realToCompare != *(float*)condition.rhsValue.data;
        case NO_OP: return true;
    }
}

bool Filter::checkScanCondition(char* varCharToCompare, Condition &condition){
    int lengthOfVarChar;
    memcpy(&lengthOfVarChar, (char*)condition.rhsValue.data, sizeof(int));
    char varCharBuffer[lengthOfVarChar+1];
    memcpy(varCharBuffer, (char*)condition.rhsValue.data+4, lengthOfVarChar);
    varCharBuffer[lengthOfVarChar] = '\0';
    // int cmp = strcmp(varCharToCompare, (char*)condition.rhsValue.data+4);
    int cmp = strcmp(varCharToCompare, varCharBuffer);
    switch(condition.op){
        case EQ_OP: return cmp == 0;
        case LT_OP: return cmp < 0;
        case GT_OP: return cmp > 0;
        case LE_OP: return cmp <= 0;
        case GE_OP: return cmp >= 0;
        case NE_OP: return cmp != 0;
        case NO_OP: return true;
    }
}

void Filter::splitLeftAttribute(const Condition &condition, string &table, string &attribute){
    string stringToParse = condition.lhsAttr;
    string delimiter = ".";
    size_t pos =stringToParse.find(delimiter);
    table = stringToParse.substr(0,pos);
    stringToParse.erase(0,pos+delimiter.length());
    attribute =stringToParse;
}

void Filter::splitRightAttribute(const Condition &condition, string &table, string &attribute){
    string stringToParse = condition.rhsAttr;
    string delimiter = ".";
    size_t pos =stringToParse.find(delimiter);
    table = stringToParse.substr(0,pos);
    stringToParse.erase(0,pos+delimiter.length());
    attribute =stringToParse;
}

Project::Project(Iterator *input, const vector<string> &attrNames){
    projectIter = input;
    tempIter = input;
    projectAttrs = attrNames;
}

RC Project::getNextTuple(void *data){
    void* returnBuffer = malloc(1000);
    char nullBuffer[1];                      //may not be able to hardcode buffer size============================
    memset(nullBuffer, 0, 1);
    memcpy((char*)data, nullBuffer, 1);

    vector<Attribute> inputAttrs;
    projectIter->getAttributes(inputAttrs);

    if(projectIter->getNextTuple(returnBuffer)){
        return QE_EOF;
    }
    int dataOffset = 1;
    for(int i = 0; i<projectAttrs.size(); i++){
        int j;
        int returnBufferOffset = 1;
        for(j = 0; j<inputAttrs.size(); j++){
            if(inputAttrs[j].name == projectAttrs[i]){
                break;
            }
            if(inputAttrs[j].type == TypeInt || inputAttrs[j].type ==TypeReal){
                returnBufferOffset += 4;
            }else{
                int lengthOfVarChar;
                memcpy(&lengthOfVarChar, (char*)returnBuffer + returnBufferOffset, sizeof(int));
                returnBufferOffset += sizeof(int) + lengthOfVarChar;
            }
        }

        if(inputAttrs[j].type == TypeInt){
            memcpy((char*)data+dataOffset, (char*)returnBuffer + returnBufferOffset, sizeof(int));
            dataOffset+=sizeof(int);
        }else if(inputAttrs[j].type == TypeReal){
            memcpy((char*)data+dataOffset, (char*)returnBuffer + returnBufferOffset, sizeof(float));
            dataOffset+=sizeof(float);
        }else{
            int lengthOfVarChar;
            memcpy(&lengthOfVarChar, (char*)returnBuffer + returnBufferOffset, sizeof(int));
            memcpy((char*)data + dataOffset, &lengthOfVarChar, sizeof(int));
            dataOffset+= sizeof(int);
            returnBufferOffset+=sizeof(int);

            // char varCharBuffer[lengthOfVarChar+1];
            char varCharBuffer[lengthOfVarChar];
            memcpy(varCharBuffer, (char*)returnBuffer + returnBufferOffset, lengthOfVarChar);
            // varCharBuffer[lengthOfVarChar] = '\0';
            memcpy((char*)data+ dataOffset, varCharBuffer, lengthOfVarChar);
            dataOffset+=lengthOfVarChar;
        }
    }
    return SUCCESS;
}

void Project::getAttributes(vector<Attribute>& attrs)const{
    attrs.clear();
    vector<Attribute> tempAttr;
    tempIter->getAttributes(tempAttr);
    
    for(int i = 0; i<projectAttrs.size(); i++){
        for(int j = 0; j<tempAttr.size(); j++){
            if(projectAttrs[i] == tempAttr[j].name){
                attrs.push_back(tempAttr[j]);
            }
        }
    }
}

INLJoin::INLJoin(Iterator *leftIn, IndexScan *rightIn, const Condition &condition){
    leftIter = leftIn;
    rightIter = rightIn;
    tempIter = rightIn;
    joinCondition = condition;
    start = false;
    leftReturn = malloc(1000);
}

RC INLJoin::getNextTuple(void *data){
    RC rc;
    vector<Attribute> leftAttrs;
    leftIter->getAttributes(leftAttrs);
    int leftNullSize = getNullIndicatorSize(leftAttrs);

    vector<Attribute> rightAttrs;
    rightIter->getAttributes(rightAttrs);
    int rightNullSize = getNullIndicatorSize(rightAttrs);

    // void* leftBuffer = malloc(1000);
    void* rightBuffer = malloc(1000);

    int nullBufferSize = getNullIndicatorSize(leftAttrs.size(), rightAttrs.size());
    char nullBuffer[nullBufferSize];
    memset(nullBuffer, 0, nullBufferSize);
    int dataOffset = nullBufferSize;

    if(start == false){
        if(leftIter->getNextTuple(leftReturn) == QE_EOF)
            return QE_EOF;
        start = true;
    }

    while(1){
        while(tempIter->getNextTuple(rightBuffer)!=QE_EOF){
            memset(rightBuffer, 0, 1000);
            int leftIndex;
            int leftOffset = leftNullSize;
            for(leftIndex = 0; leftIndex < leftAttrs.size(); leftIndex++){      //finds the index of the left.attribute and calculates the
                if(joinCondition.lhsAttr == leftAttrs[leftIndex].name){         //offset of the attribute in returned data
                    break;
                }
                if(leftAttrs[leftIndex].type == TypeInt){
                    leftOffset += sizeof(int);
                }else if(leftAttrs[leftIndex].type == TypeReal){
                    leftOffset+= sizeof(float);
                }else{
                    int sizeOfVarCharInRecord;                        
                    memcpy(&sizeOfVarCharInRecord, (char*)leftReturn + leftOffset, sizeof(int));
                    leftOffset+= sizeof(int) + sizeOfVarCharInRecord;
                }
            }

            int leftIntValue;
            float leftFloatValue;
            char* leftVarChar;

            //gets the left attribute
            if(leftAttrs[leftIndex].type == TypeInt){
                // int leftValue;
                memcpy(&leftIntValue, (char*)leftReturn + leftOffset, sizeof(int));
            }else if(leftAttrs[leftIndex].type == TypeReal){
                // float leftValue;
                memcpy(&leftFloatValue, (char*)leftReturn + leftOffset, sizeof(float));
            }else{
                int sizeOfVarChar;
                memcpy(&sizeOfVarChar, (char*)leftReturn + leftOffset, sizeof(int));
                leftOffset += sizeof(int);
                leftVarChar[sizeOfVarChar+1];
                memcpy(&leftVarChar, (char*)leftReturn + leftOffset, sizeOfVarChar);
                leftVarChar[sizeOfVarChar] = '\0';
            }

            bool validTuple = false;

            int rightIndex;
            int rightOffset = rightNullSize;
            for(rightIndex = 0; rightIndex < rightAttrs.size(); rightIndex++){      //finds the index of the right.attribute and calculates the
                if(joinCondition.rhsAttr == rightAttrs[rightIndex].name){         //offset of the attribute in returned data
                    break;
                }
                if(rightAttrs[rightIndex].type == TypeInt){
                    rightOffset += sizeof(int);
                }else if(rightAttrs[rightIndex].type == TypeReal){
                    rightOffset+= sizeof(float);
                }else{
                    int sizeOfVarCharInRecord;                              
                    memcpy(&sizeOfVarCharInRecord, (char*)rightBuffer + rightOffset, sizeof(int));
                    rightOffset+= sizeof(int) + sizeOfVarCharInRecord;
                }
            }
            //gets the right attribute
            if(rightAttrs[rightIndex].type == TypeInt){
cout<<"in type int"<<endl;
                int rightValue;
                memcpy(&rightValue, (char*)rightBuffer + rightOffset, sizeof(int));
                validTuple = returnValidTuple(leftIntValue, rightValue, joinCondition);
                if(validTuple == true){
                    memcpy((char*)data, (char*)nullBuffer, nullBufferSize);
                    int leftSize = getRecordLength(leftAttrs, leftReturn);      //does not include null buffer size
                    int rightSize = getRecordLength(rightAttrs, rightBuffer);
                    memcpy((char*)data+dataOffset, (char*)leftReturn + leftNullSize, leftSize);     
                    dataOffset+=leftSize;
                    memcpy((char*)data+dataOffset, (char*)rightBuffer+rightNullSize, rightSize);
                    return SUCCESS;
                }
            }else if(rightAttrs[rightIndex].type == TypeReal){
cout<<"in type real"<<endl;
                float rightValue;
                memcpy(&rightValue, (char*)rightBuffer + rightOffset, sizeof(float));
                validTuple = returnValidTuple(leftFloatValue, rightValue, joinCondition);
cout<<"left: "<<leftFloatValue<<endl;
cout<<"right: "<<rightValue<<endl;
cout<<"valid: "<<validTuple<<endl;
                if(validTuple == true){
                    memcpy((char*)data, (char*)nullBuffer, nullBufferSize);
                    int leftSize = getRecordLength(leftAttrs, leftReturn);
                    int rightSize = getRecordLength(rightAttrs, rightBuffer);
                    memcpy((char*)data+dataOffset, (char*)leftReturn + leftNullSize, leftSize);     
                    dataOffset+=leftSize;
                    memcpy((char*)data+dataOffset, (char*)rightBuffer+rightNullSize, rightSize);
                    return SUCCESS;
                }
            }else{
cout<<"in varChar"<<endl;
                int sizeOfVarChar;
                memcpy(&sizeOfVarChar, (char*)rightBuffer + rightOffset, sizeof(int));
                rightOffset += sizeof(int);
                char rightValue[sizeOfVarChar+1];
                memcpy(&rightValue, (char*)rightBuffer + rightOffset, sizeOfVarChar);
                rightValue[sizeOfVarChar] = '\0';
                validTuple = returnValidTuple(leftVarChar, rightValue, joinCondition);
                if(validTuple == true){
                    memcpy((char*)data, (char*)nullBuffer, nullBufferSize);
                    int leftSize = getRecordLength(leftAttrs, leftReturn);
                    int rightSize = getRecordLength(rightAttrs, rightBuffer);
                    memcpy((char*)data+dataOffset, (char*)leftReturn + leftNullSize, leftSize);     
                    dataOffset+=leftSize;
                    memcpy((char*)data+dataOffset, (char*)rightBuffer+rightNullSize, rightSize);
                    return SUCCESS;
                }
            }
        }
cout<<"incrementing left"<<endl;
        memset(leftReturn, 0, 1000);
        tempIter = rightIter;
        if(tempIter->getNextTuple(leftReturn)==QE_EOF){
cout<<"failed resetting iterator"<<endl;
            return QE_EOF;
        }
        if(leftIter->getNextTuple(leftReturn) == QE_EOF){
            return QE_EOF;
        }
cout<<"getting next left tuple"<<endl;
    }
}

void INLJoin::getAttributes(vector<Attribute> &attrs) const{
    vector<Attribute> leftAttr;
    leftIter->getAttributes(leftAttr);

    vector<Attribute> rightAttr;
    rightIter->getAttributes(rightAttr);

    for(int i =0; i<leftAttr.size(); i++){
        attrs.push_back(leftAttr[i]);
    }
    
    for(int i = 0; i<rightAttr.size(); i++){
        attrs.push_back(rightAttr[i]);
    }
}

int INLJoin::getNullIndicatorSize(vector<Attribute> &attr){
    return int(ceil((double) (attr.size()) / CHAR_BIT));
}

int INLJoin::getNullIndicatorSize(int leftAttributeSize, int rightAttributeSize) 
{
    return int(ceil((double) (leftAttributeSize+rightAttributeSize) / CHAR_BIT));
}

bool INLJoin::returnValidTuple(int left, int right, Condition condition){
    switch(condition.op){
        case EQ_OP: return left == right;
        case LT_OP: return left < right;
        case GT_OP: return left > right;
        case LE_OP: return left <= right;
        case GE_OP: return left >= right;
        case NE_OP: return left != right;
        case NO_OP: return true;
    }
}

bool INLJoin::returnValidTuple(float left, float right, Condition condition){
    switch(condition.op){
        case EQ_OP: return left == right;
        case LT_OP: return left < right;
        case GT_OP: return left > right;
        case LE_OP: return left <= right;
        case GE_OP: return left >= right;
        case NE_OP: return left != right;
        case NO_OP: return true;
    }
}

bool INLJoin::returnValidTuple(void* left, void* right, Condition condition){
    int cmp = strcmp((char*)left, (char*)right);
    switch(condition.op){
        case EQ_OP: return cmp == 0;
        case LT_OP: return cmp < 0;
        case GT_OP: return cmp > 0;
        case LE_OP: return cmp <= 0;
        case GE_OP: return cmp >= 0;
        case NE_OP: return cmp != 0;
        case NO_OP: return true;
    }
}

int INLJoin::getRecordLength(vector<Attribute> attr, void* data){
    int nullSize = getNullIndicatorSize(attr);
    int offset = nullSize;
    for(int i = 0; i<attr.size(); i++){
        if(attr[i].type == TypeInt || attr[i].type ==TypeReal){
                offset += 4;
        }else{
            int lengthOfVarChar;                            //need to adjust for null string terminator=====================
            memcpy(&lengthOfVarChar, (char*)data + offset, sizeof(int));
            offset += sizeof(int) + lengthOfVarChar;
        }
    }
    return offset - nullSize;
}