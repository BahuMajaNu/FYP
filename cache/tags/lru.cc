/*
 * Copyright (c) 2012-2013 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2003-2005,2014 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Erik Hallnor
 */

/**
 * @file
 * Definitions of a LRU tag store.
 */

#include "debug/LER.hh"
#include "debug/CacheRepl.hh"
#include "mem/cache/tags/lru.hh"
#include "mem/cache/base.hh"

#include <iostream>
#include <fstream>

LRU::LRU(const Params *p)
    : BaseSetAssoc(p)
{
	ht_00_11=0;
  	ht_01_11=0;
  	ht_10_00=0;
  	ht_11_00=0;
  	tt_00_10=0;
  	tt_01_10=0;
  	tt_11_01=0;
  	tt_10_01=0;

	//DPRINTF(LER,"set ll: selecting blk ll for replacement\n");
}

CacheBlk*
LRU::accessBlock(Addr addr, bool is_secure, Cycles &lat, int master_id)
{
    CacheBlk *blk = BaseSetAssoc::accessBlock(addr, is_secure, lat, master_id);

    if (blk != NULL) {

		/** Begin of Changes - Phase 1 */

		blk->writeCount++;
	
		/** Begin of Changes - Phase 1 */

        // move this block to head of the MRU list
        sets[blk->set].moveToHead(blk);
        DPRINTF(CacheRepl, "set %x: moving blk %x (%s) to MRU\n",
                blk->set, regenerateBlkAddr(blk->tag, blk->set),
                is_secure ? "s" : "ns");
    }

    return blk;
}

void 
LRU::countTransitionsAndEncode(const uint8_t *incoming,const uint8_t *existing,PacketPtr pkt,CacheBlk *victim){
	
	int dist=0;
	uint64_t new_data, old_data, newData;
	new_data =	(uint64_t) incoming[0] | 
			 	(uint64_t) incoming[1] << 8 | 
			  	(uint64_t) incoming[2] << 16 | 
			  	(uint64_t) incoming[3] << 24 | 
			  	(uint64_t) incoming[4] << 32 | 
			  	(uint64_t) incoming[5] << 40 |
			  	(uint64_t) incoming[6] << 48 |
			  	(uint64_t) incoming[7] << 56;			
	
	newData = new_data;
	
	if(victim->newData == nullptr){
		old_data =	(uint64_t) existing[0] | 
					(uint64_t) existing[1] << 8 | 
					(uint64_t) existing[2] << 16 | 
					(uint64_t) existing[3] << 24 | 
					(uint64_t) existing[4] << 32 | 
					(uint64_t) existing[5] << 40 |
					(uint64_t) existing[6] << 48 |
					(uint64_t) existing[7] << 56;
	}
	else
		old_data = victim->newData[0];
	
	int j = 0;
	while(j < 64){
		bool old_bit = old_data & 2;
		bool new_bit = new_data & 2;
		if( ( old_bit ^ victim->flags[j/2] ) != new_bit){ 
			dist++;
			victim->flags[j/2]=true;
			
			if(old_bit){
				old_bit = old_data & 1;
				new_bit = new_data & 1;
				if(old_bit){
					if(new_bit)
						tt_11_01++;
					else
						ht_11_00++;
				}
				else{
					if(new_bit)
						tt_10_01++;
					else
						ht_10_00++;
				}
				
			}
			else{
				old_bit = old_data & 1;
				new_bit = new_data & 1;
				if(old_bit){
					if(new_bit)
						ht_01_11++;
					else
						tt_01_10++;
				}
				else{
					if(new_bit)
						ht_00_11++;
					else
						tt_00_10++;
				}
					
			}
			
			newData = newData ^ ( 1 << ( 63 - j ) );
				
		}
		old_data = old_data >> 2;
		new_data = new_data >> 2;
		j+=2;
	}
	
	victim->newData = &newData;
	
	/*DPRINTF(LER,"ht_00_11 %d\t", ht_00_11);
   	DPRINTF(LER,"ht_01_11 %d\t", ht_01_11);
   	DPRINTF(LER,"ht_11_00 %d\t", ht_11_00);
   	DPRINTF(LER,"ht_10_00 %d\t", ht_10_00);
    	
   	DPRINTF(LER,"tt_00_10 %d\t", tt_00_10);
   	DPRINTF(LER,"tt_01_10 %d\t", tt_01_10);
   	DPRINTF(LER,"tt_11_01 %d\t", tt_11_01);
   	DPRINTF(LER,"tt_10_01 %d\n", tt_10_01);*/
	
}


int
LRU::hammingDist(const uint8_t *incoming,const uint8_t *existing){
	
	int dist=0;
	uint64_t new_data, old_data;
	new_data =	(uint64_t) incoming[0] | 
			 	(uint64_t) incoming[1] << 8 | 
			  	(uint64_t) incoming[2] << 16 | 
			  	(uint64_t) incoming[3] << 24 | 
			  	(uint64_t) incoming[4] << 32 | 
			  	(uint64_t) incoming[5] << 40 |
			  	(uint64_t) incoming[6] << 48 |
			  	(uint64_t) incoming[7] << 56;			
	
	old_data =	(uint64_t) existing[0] | 
				(uint64_t) existing[1] << 8 | 
				(uint64_t) existing[2] << 16 | 
				(uint64_t) existing[3] << 24 | 
				(uint64_t) existing[4] << 32 | 
				(uint64_t) existing[5] << 40 |
				(uint64_t) existing[6] << 48 |
				(uint64_t) existing[7] << 56;
		
	
	int j = 0;
	while(j < 64){
		bool old_bit = old_data & 2;
		bool new_bit = new_data & 2;
		
		if(old_bit != new_bit)
			dist++;		
		
		old_data = old_data >> 2;
		new_data = new_data >> 2;
		j+=2;
	}
	
	return dist;
}


CacheBlk*
LRU::findVictim(Addr addr, PacketPtr pkt, bool isTopLevel)	// change - Phase 1
{
	int curSet = extractSet(addr);
	if(isTopLevel)
		return sets[curSet].blks[assoc-1];
		
	int threshold = 8;
    
	CacheBlk *victim=nullptr; 

	//set number of the blk
	
	int minDist=33;
	const uint8_t *incoming = pkt->getConstPtr<uint8_t>();

	bool val=sets[curSet].blks[assoc-1]->isValid();

	for(int _j=0 ; _j<assoc ;_j++){
		const uint8_t *existing = sets[curSet].blks[_j]->data;

		if(val==sets[curSet].blks[_j]->isValid()  && sets[curSet].blks[_j]->counter!=threshold){
		
			int dist=hammingDist(incoming,existing);
		
			if(dist < minDist)
				minDist = dist , victim = sets[curSet].blks[_j];	
		}
		
	}
	
	if(victim==nullptr){
	
		for(int _j=0 ; _j<assoc ;_j++){
			const uint8_t *existing = sets[curSet].blks[_j]->data;

			if(val==sets[curSet].blks[_j]->isValid()){
			
				int dist=hammingDist(incoming,existing);
			
				if(dist < minDist)
					minDist = dist , victim = sets[curSet].blks[_j];	
			
				sets[curSet].blks[_j]->counter=0;
			}
						
		}
	}

	victim->counter++;
	const uint8_t *existing = victim->data;
	countTransitionsAndEncode(incoming,existing,pkt,victim);

	return victim;
}

void
LRU::insertBlock(PacketPtr pkt, BlkType *blk)
{
    BaseSetAssoc::insertBlock(pkt, blk);

    int set = extractSet(pkt->getAddr());
    sets[set].moveToHead(blk);
}

void
LRU::invalidate(CacheBlk *blk)
{
    BaseSetAssoc::invalidate(blk);

    // should be evicted before valid blocks
    int set = blk->set;
    sets[set].moveToTail(blk);
}

LRU*
LRUParams::create()
{
    return new LRU(this);
}
