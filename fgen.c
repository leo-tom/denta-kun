/*
Copyright 2020, Leo Tomura

This file is part of Dentakun.

Dentakun is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Dentakun is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Dentakun.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "denta-kun.h"

Poly _cfunc2poly(uint64_t (*funcptr)(uint64_t),size_t inputSize,size_t outputSize,BlackBoard blackboard){
	int64_t i,j;
	uint64_t x = 0;
	uint64_t max = 0;
	int end = 0;
	uint64_t outputSizeInPowerOf2 = outputSize;
	if(popcount(outputSize) != 1){
		outputSizeInPowerOf2 = (outputSize / 8 + 1) * 8;
	}
	Poly *polies = malloc(sizeof(Poly) * outputSizeInPowerOf2);
	for(i = 0;i < outputSize;i++){
		polies[i] = polyDup(zeroPoly);
	}
	for(i = 0;i < inputSize;i++){
		max |= 1 << i;
	}
	do{
		if(x == max){
			end = 1;
		}
		uint64_t val = funcptr(x);
		for(i = 0;i < outputSize;i++){
			if(val & (1 << i)){
				Poly tmp = K2Poly(K_1);
				for(j = 0; j < inputSize;j++){
					char buff[32];
					if(x & (1 << j)){
						sprintf(&buff[0],"x_{%ld}",j);
					}else{
						sprintf(&buff[0],"x_{%ld} + 1",j);
					}
					Poly mulDis = instantParser(buff,&blackboard);
					Poly _tmp = polyMul(tmp,mulDis);
					polyFree(tmp);
					polyFree(mulDis);
					tmp = _tmp;
				}
				Poly multiplied = polyMul(polies[i],tmp);
				Poly added = polyAdd(polies[i],tmp);
				Poly newVal = polyAdd(added,multiplied);
				polyFree(polies[i]);
				polies[i] = newVal;
				polyFree(tmp);
				polyFree(added);
				polyFree(multiplied);
			}
		}
		x++;
	}while(!end);
	return mkPolyArray(polies,outputSizeInPowerOf2);
}
uint64_t wrapper_64_64(uint64_t v){uint64_t (*funcptr)(uint64_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_32_64(uint64_t v){uint32_t (*funcptr)(uint64_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_16_64(uint64_t v){uint16_t (*funcptr)(uint64_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_8_64(uint64_t v){uint8_t (*funcptr)(uint64_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_64_32(uint64_t v){uint64_t (*funcptr)(uint32_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_32_32(uint64_t v){uint32_t (*funcptr)(uint32_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_16_32(uint64_t v){uint16_t (*funcptr)(uint32_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_8_32(uint64_t v){uint8_t (*funcptr)(uint32_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_64_16(uint64_t v){uint64_t (*funcptr)(uint16_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_32_16(uint64_t v){uint32_t (*funcptr)(uint16_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_16_16(uint64_t v){uint16_t (*funcptr)(uint16_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_8_16(uint64_t v){uint8_t (*funcptr)(uint16_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_64_8(uint64_t v){uint64_t (*funcptr)(uint8_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_32_8(uint64_t v){uint32_t (*funcptr)(uint8_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_16_8(uint64_t v){uint16_t (*funcptr)(uint8_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
uint64_t wrapper_8_8(uint64_t v){uint8_t (*funcptr)(uint8_t) = LOADED_FUNCTION_PTR;return funcptr(v);}
Poly cfunc2Poly(size_t inputSize,size_t outputSize,BlackBoard blackboard){
	uint64_t inputSizeInByte = inputSize;
	uint64_t outputSizeInByte = outputSize;
	if(popcount(inputSize) != 1){
		inputSizeInByte = (inputSize / 8 + 1);
	}
	if(popcount(outputSize) != 1){
		outputSizeInByte = (outputSize / 8 + 1);
	}
	void *wrappers[4][4] = {
		{ wrapper_8_8, wrapper_8_16, wrapper_8_32, wrapper_8_64},
		{ wrapper_16_8, wrapper_16_16, wrapper_16_32, wrapper_16_64},
		{ wrapper_32_8, wrapper_32_16, wrapper_32_32, wrapper_32_64},
		{ wrapper_64_8, wrapper_64_16, wrapper_64_32, wrapper_64_64}
	};
	return _cfunc2poly(wrappers[outputSizeInByte - 1][inputSizeInByte - 1],inputSize,outputSize,blackboard);
}


