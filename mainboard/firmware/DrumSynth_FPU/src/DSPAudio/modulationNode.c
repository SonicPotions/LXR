/*
 * VelocityModulation.c
 *
 *  Created on: 06.01.2013
 * ------------------------------------------------------------------------------------------------------------------------
 *  Copyright 2013 Julian Schmidt
 *  Julian@sonic-potions.com
 * ------------------------------------------------------------------------------------------------------------------------
 *  This file is part of the Sonic Potions LXR drumsynth firmware.
 * ------------------------------------------------------------------------------------------------------------------------
 *  Redistribution and use of the LXR code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *       - The code may not be sold, nor may it be used in a commercial product or activity.
 *
 *       - Redistributions that are modified from the original source must include the complete
 *         source code, including the source code for all components used by a binary built
 *         from the modified sources. However, as a special exception, the source code distributed
 *         need not include anything that is normally distributed (in either source or binary form)
 *         with the major components (compiler, kernel, and so on) of the operating system on which
 *         the executable runs, unless that component itself accompanies the executable.
 *
 *       - Redistributions must reproduce the above copyright notice, this list of conditions and the
 *         following disclaimer in the documentation and/or other materials provided with the distribution.
 * ------------------------------------------------------------------------------------------------------------------------
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ------------------------------------------------------------------------------------------------------------------------
 */



#include "modulationNode.h"
#include "DrumVoice.h"
#include "CymbalVoice.h"
#include "HiHat.h"
#include "Snare.h"
#include "sequencer.h"

 ModulationNode velocityModulators[6];

 //-----------------------------------------------------------------------
void modNode_init(ModulationNode* vm)
{
	vm->lastVal = 0;
	vm->amount = 0.f;
	modNode_setDestination(vm,0);
}
//-----------------------------------------------------------------------
static void modNode_setOriginalValueChanged(ModulationNode* vm, uint16_t idx)
{
	// --AS TODO this code is repeated (approximately. not exactly) inside modNode_setDestination. can we combine?


	if(vm->destination == idx) { //parameter is active in this modulator
		const Parameter const *p=&parameterArray[idx];

			//update orig value
			switch(p->type)
			{
				case TYPE_UINT8:
					vm->originalValue.itg = *((uint8_t*)p->ptr);
					break;

				case TYPE_SPECIAL_F:
					break;

				case TYPE_SPECIAL_P:
				case TYPE_SPECIAL_FILTER_F:
				case TYPE_FLT:
					vm->originalValue.flt = *((float*)p->ptr);
					break;

				case TYPE_UINT32:
					vm->originalValue.itg = *((uint32_t*)p->ptr);
					break;

				default:
					break;
			}
		}
}
//-----------------------------------------------------------------------
// This is called when a user changes a parameter value on the front. It saves
// the new value as originalValue. Since the value changes as modulation happens,
// we need to restore to the original value from time to time
void modNode_originalValueChanged(uint16_t idx)
{
	uint8_t i;
	for(i=0;i<6;i++)
	{
		modNode_setOriginalValueChanged(&velocityModulators[i],idx);
	}

	modNode_setOriginalValueChanged(&voiceArray[0].lfo.modTarget,idx);
	modNode_setOriginalValueChanged(&voiceArray[1].lfo.modTarget,idx);
	modNode_setOriginalValueChanged(&voiceArray[2].lfo.modTarget,idx);
	modNode_setOriginalValueChanged(&snareVoice.lfo.modTarget,idx);
	modNode_setOriginalValueChanged(&cymbalVoice.lfo.modTarget,idx);
	modNode_setOriginalValueChanged(&hatVoice.lfo.modTarget,idx);
}
//-----------------------------------------------------------------------
void modNode_resetTargets()
{
	uint8_t i;
	for(i=0;i<6;i++)
	{
		paramArray_setParameter(velocityModulators[i].destination,velocityModulators[i].originalValue);
	}

	paramArray_setParameter(voiceArray[0].lfo.modTarget.destination,voiceArray[0].lfo.modTarget.originalValue);
	paramArray_setParameter(voiceArray[1].lfo.modTarget.destination,voiceArray[1].lfo.modTarget.originalValue);
	paramArray_setParameter(voiceArray[2].lfo.modTarget.destination,voiceArray[2].lfo.modTarget.originalValue);
	paramArray_setParameter(snareVoice.lfo.modTarget.destination,snareVoice.lfo.modTarget.originalValue);
	paramArray_setParameter(cymbalVoice.lfo.modTarget.destination,cymbalVoice.lfo.modTarget.originalValue);
	paramArray_setParameter(hatVoice.lfo.modTarget.destination,hatVoice.lfo.modTarget.originalValue);


}
//-----------------------------------------------------------------------
void modNode_reassignVeloMod()
{
	uint8_t i;
	for(i=0;i<6;i++)
	{
		modNode_updateValue(&velocityModulators[i], velocityModulators[i].lastVal);
	}
}
//-----------------------------------------------------------------------
// set a modulation destination to one of the sound parameters.
// This is called when the mod target changes or is initialized.
// The target's actual value needs to be preserved because it will be modulated.
void modNode_setDestination(ModulationNode* vm, uint16_t dest)
{
	//TODO check if this interrupts other modulations too much
	//is needed to really get the original value
	modNode_resetTargets();

	//restore old value to original --AS TODO isn't this already being done above for all nodes?
	paramArray_setParameter(vm->destination,vm->originalValue);

	//update dest param
	vm->destination = dest;

	const Parameter const *p = &parameterArray[vm->destination];

	//--AS **PATROT we want to avoid reading from invalid memory
	if(!p->ptr)
		return;

	//get new original parameter value
	switch(p->type)
	{
		case TYPE_UINT8:
			vm->originalValue.itg = *((uint8_t*)p->ptr);
			break;
		case TYPE_SPECIAL_F: // --AS TODO whats up with this???
			vm->originalValue.flt = 1;//*((float*)parameterArray[vm->destination].ptr);
			break;

		case TYPE_SPECIAL_FILTER_F:
		case TYPE_FLT:
			vm->originalValue.flt = *((float*)p->ptr);
			break;

		case TYPE_UINT32:
			vm->originalValue.itg = *((uint32_t*)p->ptr);
			break;

		case TYPE_SPECIAL_P:
		default:
			break;
	}
}
//-----------------------------------------------------------------------
// This is called to actually modulate the value for a modulation node
void modNode_updateValue(ModulationNode* vm, float val)
{
	Parameter const *p = &parameterArray[vm->destination];

	vm->lastVal = val;

	// --AS **PATROT avoid setting this if it's not set to something good
	if(!p->ptr)
		return;

	switch(p->type)
	{
	case TYPE_UINT8:
		(*((uint8_t*)p->ptr)) = (*((uint8_t*)p->ptr)) * vm->amount * val + (1.f-vm->amount) * (*((uint8_t*)p->ptr));
		break;

	case TYPE_UINT32:
		(*((uint32_t*)p->ptr)) = (*((uint32_t*)p->ptr)) * vm->amount * val + (1.f-vm->amount) * (*((uint32_t*)p->ptr));
		break;

	case TYPE_FLT:
		(*((float*)p->ptr)) = (*((float*)p->ptr)) * vm->amount * val + (1.f-vm->amount) * (*((float*)p->ptr));
		break;

	case TYPE_SPECIAL_F:
		(*((float*)p->ptr)) = (*((float*)p->ptr)) * vm->amount * val + (1.f-vm->amount) * (*((float*)p->ptr));
		break;

	case TYPE_SPECIAL_P:
	case TYPE_SPECIAL_FILTER_F:
	default:
		break;

	}
}

