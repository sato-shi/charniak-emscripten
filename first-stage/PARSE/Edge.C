/*
 * Copyright 1999, 2005 Brown University, Providence, RI.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.  You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include "Edge.h"
#include "Item.h"	// need Item, Items definitions
#include "GotIter.h"
#include "Bchart.h"
#include <math.h>
#include "Feature.h"

#define DEMFAC .999
#define EDGE_CHUNKSIZE		1000
//int Edge::numEdges = 0;

//Edge *	Edge::edge_cache_;


Edge::
~Edge()
{   
  //numEdges--;
}

bool
Edge::
check()
{
  GotIter gi(this); 
  Item* f;
  while(gi.next(f))
    assert(f->finish() == 0 || f->finish() > f->start());
  return true;
}

int
Edge::
ccInd()
{
  const Term* trm = lhs();
  int tint = trm->toInt();
  ECString tNm = trm->name();
  bool sawComma = false;
  bool sawColen = false;
  bool sawCC = false;
  int numTrm = 0;
  Item* itm;
  LeftRightGotIter gi(this);  
  int pos = 0;
  /*Change next line to indicate which non-terminals get specially
    marked to indicate that they are conjoined together */
  if(!trm->isNP() && !trm->isS() && !trm->isVP()) return tint;
  while( gi.next(itm) )
    {
      const Term* subtrm = itm->term();
      if(subtrm == Term::stopTerm) continue;
      if(subtrm == trm) numTrm++;

      if(pos != 0 && subtrm->isCC()) sawCC = true;
      if(pos != 0 && subtrm->isComma()) sawComma = true;
      if(pos != 0 && subtrm->isColon()) sawColen = true;
      
      pos++;
    }
  if(trm->isNP() && numTrm == 2 && !sawCC) return Term::lastNTInt()+1;
  if((sawComma || sawColen || sawCC) && numTrm >= 2)
    return tint+Term::lastNTInt();
  return tint;
}


Edge::
Edge(ConstTerm* trm)
  :
  lhs_( trm ),
  loc_( -1 ),
  finishedParent_( NULL ),
  pred_(NULL),
  start_(-1),
  num_(-1),
  status_(0),
  item_(NULL),
  heapPos_(-1),
  demerits_(0),
  prob_(1.2) // encourage constits???
{
  if(lhs_->isVP()) prob_ *= 1.4; //???;
  // VPs are badly modeled by system;  This is only called during first parse.
}

Edge::
Edge( Edge& src, Item& itm, int right )
: lhs_( src.lhs_ ),
  loc_( src.loc_ ),
  finishedParent_( NULL ),
  start_(src.start_),
  num_(-1),
  status_(right),
  item_(&itm),
  heapPos_(-1),
  demerits_(src.demerits_),
  leftMerit_(src.leftMerit()),
  rightMerit_(src.rightMerit()),
  prob_(src.prob())
{
  //numEdges++;
  if(start_ == -1)
    start_ = itm.start();
  if(loc_ == -1) loc_ = itm.finish();
  if(right) loc_ = itm.finish();
  else start_ = itm.start();

  if(!src.item_) //it has no item
    {
      pred_ = NULL;
    }
  else
    {
      pred_ = &src;
      pred_->sucs_.push_front(this);
      //cerr << *pred_ << " has suc " << *this << endl;
    }
  prob_ *= itm.prob();
}

Edge::
Edge( Item& itm )
: 
  lhs_( itm.term() ),
  loc_( itm.finish() ),
  finishedParent_( &itm ),
  pred_(NULL),
  start_(itm.start()),
  num_(-1),
  status_(2),
  item_(NULL),
  heapPos_(-1),
  demerits_(0),
  leftMerit_(1),
  rightMerit_(1),
  prob_( itm.prob() )
{
  //numEdges++;
}

void 
Edge::
print( ostream& os )
{
    if(!item_) //dummy rule
      {
	if(finishedParent_)
	  {
	    os << *finishedParent_ << " -> ";
	  }
	else
	  {
	    os << *lhs_ << " -> ";
	  }
      }
    else 
    {
        os << *lhs_ << "(" << start() << ", " << loc_ << ") -> ";
	LeftRightGotIter gi(this);
	Item* itm;
	while( gi.next(itm) )
	  {
	    if(itm->term() == Term::stopTerm) continue;
	    os << *itm << " ";
	  }
    }
}

void
Edge::
setmerit()
{
  merit_ = prob_*leftMerit_*rightMerit_*pow(DEMFAC,demerits_);
}

Item*
Edge::
headItem()
{
  GotIter gotiter(this);
  Item* ans = NULL;
  Item* next = NULL;
  while(gotiter.next(next))  //the head will be the the last thing in gotiter;
    ans = next;
  return ans;  
}

int
Edge::
headPos(int i /*=0*/)
{
  if(!pred()) return i;
  Edge* prd = pred();
  //cerr << "H " << *item() << endl;
  if(prd->start() > start()) return prd->headPos(i+1);
  else if(prd->start() == start()
	  && item()->term() == Term::stopTerm
	  && item()->start() == start())
    {
      //cerr << "HPST " << *(prd->item()) << " " << i << endl;
      return prd->headPos(i+1);
    }
  else return pred()->headPos(i);
}
