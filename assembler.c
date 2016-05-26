#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>

#include "assembler.h"
#include "field.h"
#include "lc3.h"
#include "symbol.h"
#include "tokens.h"
#include "util.h"

/** Provide prototype for strdup() */
char *strdup(const char *s);

/** Global variable containing the head of the linked list of structures */
static line_info_t* infoHead;

/** Global variable containing the tail of the linked list of structures */
static line_info_t* infoTail;

/** Global variable containing information about the current line */
static line_info_t* currInfo;

void asm_init_line_info (line_info_t* info) {
  if (info) {
    info->next        = NULL;
    info->lineNum     = srcLineNum;
    info->address     = currAddr;
    info->machineCode = 0;
    info->opcode      = OP_INVALID;
    info->form        = 0;
    info->reg1        = -1;
    info->reg2        = -1;
    info->reg3        = -1;
    info->immediate   = 0;
    info->reference   = NULL;
  }
}

void asm_print_line_info (line_info_t* info) {
  if (info) {
    printf("%3d: address: x%04x machineCode:%04x op: %s form: %d reg1:%3d, reg2:%3d reg3:%3d imm: %d ref: %s\n",
           info->lineNum, info->address, info->machineCode,
           lc3_get_opcode_name(info->opcode), info->form, info->reg1,
           info->reg2, info->reg3, info->immediate, info->reference);
  }
 }

/* based on code from http://www.eskimo.com/~scs/cclass/int/sx11c.html */
void asm_error (char* msg, ...) {
   numErrors++;
   va_list argp;
   fprintf(stderr, "ERROR %3d: ", srcLineNum);
   va_start(argp, msg);
   vfprintf(stderr, msg, argp);
   va_end(argp);
   fprintf(stderr, "\n");
}

void asm_init (void) {
  infoHead = infoTail = currInfo = NULL; 
  tokens_init();
  lc3_sym_tab = symbol_init(0); 
}

void asm_pass_one (char* asm_file_name, char* sym_file_name) {
	//1. open the source file and report an error 	
	FILE *sourceFile = open_read_or_error(asm_file_name);
	if(sourceFile == NULL) return;  //???
	//2. read the lines one at a time using fgets
	char str[8180]; //lines
	char *curr; //tokens
	while(fgets(str, 8180, sourceFile) != NULL){
		srcLineNum++; 
		//3. convert the line to a list of tokenss
		curr = tokenize_line(str); 
		//printf("NUMTOKENS in line: %d\n", token_count());printf("READING in line: %s\n", str);
		//printf("CURRENT tokens: %s\n", curr);	
		//print_tokens();	
		if(curr != NULL) { //if there are any tokens on the line
			//4.1 allocate a new line_info_t store it in the global variable currInfo			
			currInfo = (line_info_t*)calloc(1,sizeof(line_info_t));		//FREE IT!!!!	
			//and initialize it		
			asm_init_line_info(currInfo);	
			//4.2 convert the source line to a list of tokens
			//4.3 convert the tokens to values and set the appropriate fields of currInfo
			currInfo -> lineNum = srcLineNum;
			check_line_syntax(curr);				
			//4.4 add it to the linked list defined by infoHead and infoTail
			if(infoHead == NULL){ //empty list
				infoHead = infoTail = currInfo;
			}else{
				infoTail-> next = currInfo;
				infoTail = currInfo;
			}
			//4.5 update the current address
			update_address();
		}
	}
	//5 If there were no errors, write the symbol table file using lc3_write_sym_tab().
	if(numErrors == 0){
		FILE *symFile = open_write_or_error(sym_file_name);		
		if(symFile != NULL){	
			lc3_write_sym_table(symFile);
		}
		fclose(symFile);
	}
	
	 
	fclose(sourceFile);
}



/** This function generates the object file. It is only called if no errors were
 *  found during <code>asm_pass_one()</code>. The basic structure of this code
 *  is to loop over the data structure created in <code>asm_pass_one()</code>,
 *  generate object code (16 bit LC3 instructions) and write it to the object
 *  file.
 *  @param obj_file_name - name of the object file for this source code
 */
void asm_pass_two (char* obj_file_name) {
	FILE *objFile = open_write_or_error(obj_file_name);
	if(objFile == NULL) return;

	currInfo = infoHead;
	while(currInfo != NULL){

		update_address();	
		//get operands  
		if(currInfo -> opcode != OP_INVALID && currInfo -> opcode != OP_END){ //if there's opcode
		//no need to write .END and labels in the hex file
			if(currInfo -> opcode == OP_BLKW){ //special case block word
				for(int i = 0; i < currInfo -> immediate; i++){
					//fprintf(objFile, "%.4x\n", 0);
					lc3_write_LC3_word (objFile, 0);
				}
			}else{
				LC3_inst_t* info = lc3_get_inst_info(currInfo -> opcode);  //get sytax information
				operand_t allOp = info -> forms[currInfo -> form].operands;

				//fix PUSH prototype error
				if(!strcasecmp(info->forms[currInfo -> form].name, "PUSH")){
					info -> forms[currInfo -> form].prototype = 0xd020;		
				}				
				
				//load in prototype
				currInfo -> machineCode = info -> forms[currInfo -> form].prototype;
			
				//loop through to encode operands
				for (operand_t op = FMT_R1; op <= FMT_STR; op <<= 1) {
					if(allOp & op){//if the operand type is used by this LC3 instuction,
						encode_operand(op);//set machinacode field	
					}
				}

				//special case for BR
				if(!strcasecmp(info->forms[0].name, "BR")){
					encode_operand(FMT_CC);
				}	
				//print to file
				//fprintf(objFile, "%.4x\n", currInfo -> machineCode); //?
				lc3_write_LC3_word (objFile, currInfo -> machineCode);
				//move to next
				}
			}
		asm_print_line_info(currInfo);
		currInfo = currInfo -> next;
	}
	fclose(objFile);
}

/** @todo implement this function */
void asm_term (void) {
	symbol_term(lc3_sym_tab);
	currInfo = infoHead;
		while(currInfo != NULL){
			line_info_t* next = currInfo -> next;
			if(currInfo -> reference != NULL){//had a reference
				free(currInfo -> reference);
			}
			free(currInfo);
			currInfo = next;
		}
	tokens_term();
}

/** @todo implement this function 
if the token is not a valid opcode, assume it is label. 
If it is a valid label, add it to the symbol table. Report any errors found using asm_error().
If the token is a label, then return the next token. Otherwise, return the parameter.
*/
char* check_for_label (char* token) {
	if(util_get_opcode(token) == -1){  //not valid opcode 
		if(util_is_valid_label(token)){ //if it's valid label
			if((symbol_find_by_name(lc3_sym_tab, token)) == NULL){
				symbol_add(lc3_sym_tab, token, currAddr); //add it to symbol table
				return next_token();
			}else{ //duplicate label
				asm_error(ERR_DUPLICATE_LABEL, token);
			}
		}else{ //not valid label
			asm_error(ERR_BAD_LABEL, token);
		}
	} //else: it is a valid opcode
  return token;
}

/** @todo implement this function */
/** A function to check the syntax of a source line. At the conclusion of this
 *  function, the appropriate fields of the global currInfo are initialized. 
 *  <ol>
 
 *  <li>if this opcode has operands associated with it then
 *         call <code>scan_operands()</code></li>
 *  <li>make sure there are no extra operands</li>
 *  </ol> 
 *  @param token - the first token on the line. This could be a label or an
 *  operator (e.g. <code>ADD</code> or <code>.FILL</code>).
 */
void check_line_syntax (char* token) {
  int errorCount   = numErrors;
	printf("check_line_syntax('%s')\n", token);
	//determine if the first token is a label
	char* nextOne = check_for_label(token); //nextOne should be the opcode (If token is a label,return next token)
	if (errorCount != numErrors){ //error inserting label 
      return;
	}	
	if(nextOne == NULL){
		return; //the line only contains the label
	}else if(util_get_opcode(nextOne) == -1){ //invalid opcode
		asm_error(ERR_MISSING_OP, nextOne);
		return;
	}else{ //valid opcode
		currInfo -> opcode = util_get_opcode(nextOne); //store opcode(int)
		LC3_inst_t* info = lc3_get_inst_info(currInfo -> opcode);  //get sytax information
		if(info == NULL){
			asm_error(ERR_MISSING_OP, currInfo -> opcode);
		}else{
			//decide form
			if (info->forms[1].name == NULL){ //only has one form
				currInfo -> form = 0;
			}
			//decide forms based on names
			else if(!strcasecmp(info->forms[0].name, nextOne)){ //same name
				//printf("found to be form zero");
				currInfo -> form = 0;
			}else if(!strcasecmp(info->forms[1].name, nextOne)){
				currInfo -> form = 1;
			}
			//fix the BR error
			if(!strcasecmp(info->forms[0].name, "BR")){
				//get nzp string
				char nzp[4];
				int s = 2;
				while(nextOne[s] != '\0'){
					nzp[s - 2] = nextOne[s];
					s++;
				}
				nzp[s - 2] = '\0';
				//info->forms[0].operands = (FMT_PCO9 | FMT_CC);
				//printf("-----fix the BR error, token is %s\n", token);
				get_operand(FMT_CC, nzp);
			}

		//To determine the number of operands an instruction has, simply count the 1 bits in the word
		if(info->forms[currInfo -> form].operands != 0){ //has opcode
				scan_operands((info -> forms[currInfo -> form]).operands);//TODO	
		}
		/*seeLC3
		printf("---------\nform bit:%2d\n", info->formBit);
      for (int i = 0; i < 2; i++) {
        if (info->forms[i].name != NULL) {
          printf("form: %d name: %s operands: %s prototype x%04x\n", i,
	       info->forms[i].name,
	       lc3_get_format_name(info->forms[i].operands),
	       info->forms[i].prototype); 
		   printf("OPERANDS: %d\n", info->forms[i].operands);
		}
	}	*/	
	}
  }
}

/** @todo implement this function */
/** A second pass function to take one field from the <code>currInfo</code>
 *  structure and place it in the <code>machineCode</code> field. For example, if
 *  the operand is <code>FMT_R1</code>, then the field <code>reg1</code> is put
 *  in bits 11 .. 9. The flow of this function mirrors the code of pass one
 *  function get_operand() 
 *  @param operand - the type of operand
 */
void encode_operand (operand_t operand) {
	  switch (operand) {
    case FMT_R1: 
      currInfo-> machineCode = setField (currInfo-> machineCode, 11, 9, currInfo->reg1);
      break;
	case FMT_R2:
      currInfo-> machineCode = setField (currInfo-> machineCode, 8, 6, currInfo->reg2);
      break;
	case FMT_R3:
      currInfo-> machineCode = setField (currInfo-> machineCode, 2, 0, currInfo->reg3);
      break;
	case FMT_CC:
      currInfo-> machineCode = setField (currInfo-> machineCode, 11, 9, currInfo->reg1);
      break;
	case FMT_IMM5:
      currInfo-> machineCode = setField (currInfo-> machineCode, 4, 0, currInfo->immediate);
      break;
	case FMT_IMM6:
      currInfo-> machineCode = setField (currInfo-> machineCode, 5, 0, currInfo->immediate);
      break;
	case FMT_VEC8:
	case FMT_ASC8:
	  currInfo-> machineCode = setField (currInfo-> machineCode, 7, 0, currInfo->immediate);
      break;

	case FMT_PCO9:  
		encode_PC_offset_or_error(9);
      break;
	case FMT_PCO11:
      	encode_PC_offset_or_error(11);
      break;
	case FMT_IMM16:
		currInfo-> machineCode = setField (currInfo-> machineCode, 15, 0, currInfo->immediate);
		/*
		if(currInfo -> reference != NULL){ //.fill followed by label
			symbol_t* symInfo = symbol_find_by_name(lc3_sym_tab, currInfo -> reference);
				if(symInfo== NULL){ //symbol not found
					asm_error(ERR_MISSING_LABEL, currInfo -> reference);
				}else{
					currInfo-> machineCode  = symInfo -> addr;
				}
		}else{
			currInfo-> machineCode = setField (currInfo-> machineCode, 15, 0, currInfo->immediate);
		}
		*/
      break;
	case FMT_STR:
      	//used by .STRINGZ, 
      break;
    default:
      break;
  }
}

/** @todo implement this function */
/** This second pass function is used to convert the reference into a PC offset.
 *  There are several errors that may occur. The reference may not occur in
 *  the symbol table, or the offset may be out of range. If successful, this
 *  function puts the PC offset in the <code>machineCode</code> field of
 *  <code>currInfo</code>.
 *  @param width - the number of bits that hold the PC offset
 */
void encode_PC_offset_or_error (int width) {
	symbol_t* symInfo = symbol_find_by_name(lc3_sym_tab, currInfo -> reference);
	if(symInfo== NULL){ //symbol not found
		asm_error(ERR_MISSING_LABEL, currInfo -> reference);
	}else{
		int offset = symInfo -> addr - currAddr;
		//printf("----current address is: %.4x, symaddr is: %x, offset is %d", currAddr,symInfo -> addr,  offset);
		if(fieldFits(offset, width, 1) == 0) { //does not fit
			asm_error(ERR_BAD_PCOFFSET, currInfo -> reference);
		}else{ //it fits! valid PC offset
			currInfo-> machineCode = setField (currInfo -> machineCode, width - 1, 0, offset);
		}
	}

}

/** A convenience function to make sure the next token is a comma and report
 *  an error if it is not.
 */
void get_comma_or_error (void) {
	char *test = next_token();
	if(!strcmp(test,",")){
		return;
	}else{
		asm_error(ERR_EXPECTED_COMMA, test);
	}
}

/** A convenience function to convert an token to an immediate value. Used for
 *  the imm5/offset6/trapvect8/.ORIG values. The value is obtained by calling a
 *  function provided for you in the lc3 module. If
 *  the value is not in the correct format, or out of range, report an error
 *  using asm_error(). If it is good, store it in the immediate field of currInfo
 *  @param token - the string to be converted to an immediate
 *  @param width - how many bits are used to store the value
 *  @param isSigned - specifies if number is signed or unsigned
 */
void get_immediate_or_error (char* token, int width, int isSigned) {
	int value;
	//printf("====get in result: %d, value: %d====\n ", lc3_get_int(token, &value), value);
	if(lc3_get_int(token, &value)){ //return 1 represents success on converting the token to number
		if(fieldFits(value, width, isSigned)){//fits!
			currInfo->immediate = value;
		}else{
			asm_error(ERR_IMM_TOO_BIG,token);
		}
	}else{// failed to convert token to number
		asm_error(ERR_BAD_IMM,token);
	}
}


/** A convenience function to get the label reference used in the
 *  <code>BR/LD/LDI/LEA/ST/STI/JSR</code> instructions.
 *  The code should make sure it is a
 *  valid label. If it is valid, store it in the reference field of currInfo. 
 *  If is not a valid label, report an error. 
 *  @param token - the reference to check
 */
void get_PC_offset_or_error (char* token) {
	if(util_is_valid_label(token)){ //string is a valid label
		currInfo->reference = strdup(token); //TODO free it !
	}else{
		asm_error(ERR_BAD_LABEL,token);
	}
		
		/*int value;
		if(lc3_get_int(token, &value)){ //return 1 represents success on converting the token to number
			currInfo->immediate = value;
		}else{
			asm_error(ERR_BAD_IMM,token);*/
}

/** A function to convert a string to a register number and report an error
 *  if the string does not represent an register.  Use the function asm_error()to report errors.
 *  @param token - the string to convert to a register
 *  @return the register number, or -1 on an error
 */
int get_reg_or_error (char *token) {
  int reg = util_get_reg(token);
  if(reg == -1){
	asm_error(ERR_EXPECTED_REG, token);
	return -1;
	}
return reg;
}

/** @todo implement this function 
convert the token to a value and store it in the currInfo data structure.
*/
void get_operand (operand_t operand, char* token) {
	//int value;
	switch (operand) {
		case FMT_R1:
			currInfo->reg1 = get_reg_or_error(token);
			break;
		case FMT_R2:
			currInfo->reg2 = get_reg_or_error(token);
			break;
		case FMT_R3:
			if(util_get_reg(token) == -1){ //not a register, try imm5
				get_immediate_or_error(token, 5, 1);
				currInfo -> form = 1;
			}else{
				currInfo->reg3 = get_reg_or_error(token);
			}
			break;
		case FMT_CC:
			//printf("-----fix the BR error, inside FMTCC, token is %s\n", token);
			//printf("---parse cond result is %d", util_parse_cond(token));
			currInfo->reg1 = util_parse_cond(token);
			break;
		case FMT_IMM5:
			get_immediate_or_error (token, 5, 1);
			break;
		case FMT_IMM6:
			get_immediate_or_error (token, 6, 1);
			break;
		case FMT_VEC8:
		case FMT_ASC8:
			get_immediate_or_error (token, 8, 0);
			break;

		case FMT_PCO9:  //difference?
			get_PC_offset_or_error(token);
			break;
		case FMT_PCO11:
			get_PC_offset_or_error(token);
			break;
		case FMT_IMM16:
			get_immediate_or_error (token, 16, 0);
			/*
			if((lc3_get_int(token, &value))){ //imm16 is a number
				get_immediate_or_error (token, 16, 0);
			}else{//.fill is followed by label
				get_PC_offset_or_error(token);	
			}		*/	
			break;
		case FMT_STR:
      	//used by .STRINGZ, 
			break;
	default:
      break;
  }
}

// can use ints instead of unsigned ints as long as v is never negative. This
// is true for the use in LC3 assembler.
// see http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
//Counting bits in an integer requires you to write a loop from 0 to 31 (inclusive)
// that checks each bit. If the bit is 1, count it, otherwise don't.
int count_bits (int v) {
	int count = 0;
  for(int i = 0; i <= 31; i++){
	if(v & 1<<i){ //bit is set
		count++;
	}
  }
  return count;
}

/** @todo implement this function */
/** A convenience function to scan all the operands of an LC3 instruction.
 *  @param operands - a "list" of the operands for the current LC3 instruction
 *  encoded as individual bits in an integer.
 */
void scan_operands (operands_t operands) {
  printf("scan_operands() for %s\n", lc3_get_format_name(operands));
  //printf("operand name: %d \n" , operands);
  int operandCount = 0;
  //initialize a count of the number of operands expected
  int numOperands  = count_bits(operands);
 // printf("NUMOPERNDS: %d\n", numOperands);
  //initialize a count of the number of errors found before this function was called.
  int errorCount   = numErrors;

  //loop over the all the possible operand types
  for (operand_t op = FMT_R1; op <= FMT_STR; op <<= 1) {
    if (errorCount != numErrors){
      return;
	}	// error, so skip processing remainder of line
	if(operands & op){//if the operand type is used by this LC3 instuction,
		//get the next token and convert it to an operand
		//printf("---Operant type found: %d---\n", op);
		char* curr = next_token();
		if(curr != NULL){
			get_operand(op, curr);
		}else{ //run out of tokens
			asm_error(ERR_MISSING_OPERAND);
		}
		if (errorCount != numErrors){//On error, simply return,continuing with the next source line
			return;
			//The errors include an operand of the wrong type, or a missing operand.
		}else{
			operandCount++;
			if(operandCount < numOperands){
			get_comma_or_error();
		}
	}  
	
	}
	//If there are additional operands, continue to scan them??
  }
  char* extra = next_token();
  if(extra != NULL){
  asm_error(ERR_EXTRA_OPERAND, extra);
  }
}

/** @todo implement this function */
/** This function is responsible for determing how much space and LC3 
 *  instruction or pseudo-op will take and updating the global variable
 *  <code>currAddr</code> defined in the file <code>assembler.h</code>.
 *  Most LC3 instructions require a single word. However, there are several
 *  exceptions:
 *  <ul>
 *  <li>a line containing only a label does not change <code>currAddr</code></li>
 *  <li><code>.ORIG</code> replaces the value of <code>currAddr</code> by
 *  its operand</li>
 *  <li><code>.BLKW</code> uses the number of words specified by its operand</li>
 *  <li><code>.STRINGZ</code> uses the length of the string minus one words.
        Make sure you understand how many word(s) a string uses.</li>
 *  <li>Some pseudo ops may expand to multiple LC3 instructions. For example,
 *  if the was a <code>.PUSH</code> pseudo-op, it would use two words.
 *  </ul>
 */
void update_address (void) {
	//.ORIG replaces the value of <code>currAddr</code> by its operand
	if(currInfo->opcode == OP_ORIG){
		currAddr = currInfo -> immediate;
	}
	//.BLKW uses the number of words specified by its operand
	else if(currInfo->opcode == OP_BLKW){
		currAddr += currInfo -> immediate;
	}
	//a line containing only a label does not change currAddr
	else if(currInfo->opcode != OP_INVALID){
		currAddr++;
	}
}


/** Open file for reading and report an error on failure. 
 * Use the C function fopen() and report errors using asm_error()
 *  @param file_name - name of file to open
 *  @return the file or NULL on error
 */
FILE *open_read_or_error (char* file_name) {
	FILE *filePointer = fopen(file_name,"r");
	if(filePointer == NULL){
		asm_error(ERR_OPEN_READ, file_name);
		return NULL;
	}else
		return filePointer;  
}

/** Open file for writing and report an error on failure. 
 *  @param file_name - name of file to open
 *  @return the file or NULL on error
 */
FILE *open_write_or_error (char* file_name) {
  FILE *filePointer = fopen(file_name,"w");
	if(filePointer == NULL){
		asm_error(ERR_OPEN_WRITE, file_name);
		return NULL;
	}else
		return filePointer;  
}

//------------------------------------Reference-------------------------------
/*
fgets FUNCTION:
char *fgets(char *str, int n, FILE *stream)
str -- This is the pointer to an array of chars where the string read is stored.
n -- This is the maximum number of characters to be read, usually size of the str
stream -- This is the pointer to a FILE object
RETURN:
Success: the function returns the same str parameter.
Failure: If the End-of-File is encountered and no characters have been read, return NULL pointer

Enumerated Types allow us to create our own symbolic names for a list of related ideas. 
enum Boolean 
        { 
          false, 
          true 
        }; 
i.e. a variable of type Boolean can take two value: t or f
enum Security_Levels 
        { 
          black_ops, 
          top_secret, 
          secret, 
          non_secret 
        }; 
usage: Security_Levels my_security_level = top_secret; 
 

*/
