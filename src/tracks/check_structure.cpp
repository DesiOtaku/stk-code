//  $Id: check_structure.cpp 1681 2008-04-09 13:52:48Z hikerstk $
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009  Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "karts/kart.hpp"
#include "modes/world.hpp"
#include "race/race_manager.hpp"
#include "tracks/check_manager.hpp"
#include "tracks/check_structure.hpp"


CheckStructure::CheckStructure(CheckManager *check_manager, 
                               const XMLNode &node, unsigned int index)
{
    m_index              = index;
    m_check_manager      = check_manager;
    std::string kind;
    node.get("kind", &kind);
    if(kind=="lap")
        m_check_type = CT_NEW_LAP;
    else if(kind=="activate")
        m_check_type = CT_ACTIVATE;
    else if(kind=="toggle")
        m_check_type = CT_TOGGLE;
    else if(kind=="ambient-light")
        m_check_type = CT_AMBIENT_SPHERE;
    else
    {
        printf("Unknown check structure '%s' - ignored.\n", kind.c_str());
    }
    m_activate_check_index = -1;
    node.get("other-id", &m_activate_check_index);
    if( (m_check_type==CT_TOGGLE || m_check_type==CT_ACTIVATE) &&
        m_activate_check_index==-1)
    {
        printf("Unknown other-id in checkline.\n");
    }
    m_active_at_reset=true;
    node.get("active", &m_active_at_reset);
}   // CheckStructure

// ----------------------------------------------------------------------------
/** Initialises the 'previous positions' of all karts with the start position
 *  defined for this track.
 *  \param track The track object defining the start positions.
 */
void CheckStructure::reset(const Track &track)
{
    m_previous_position.clear();
    m_is_active.clear();
    World *world = World::getWorld();
    for(unsigned int i=0; i<world->getNumKarts(); i++)
    {
        const Vec3 &xyz = world->getKart(i)->getXYZ();
        m_previous_position.push_back(xyz);
        
        // Activate all checkline
        m_is_active.push_back(m_active_at_reset);
    }   // for i<getNumKarts
}   // reset

// ----------------------------------------------------------------------------
/** Updates all check structures. Called one per time step.
 *  \param dt Time since last call.
 */
void CheckStructure::update(float dt)
{
    World *world = World::getWorld();
    for(unsigned int i=0; i<world->getNumKarts(); i++)
    {
        const Vec3 &xyz = world->getKart(i)->getXYZ();
        // Only check active checklines.
        if(m_is_active[i] && isTriggered(m_previous_position[i], xyz, i))
        {
            if(UserConfigParams::m_check_debug)
                printf("CHECK: Check structure %d triggered for kart %s.\n",
                       m_index, world->getKart(i)->getIdent().c_str());
            trigger(i);
        }
        m_previous_position[i] = xyz;
    }   // for i<getNumKarts
}   // update

// ----------------------------------------------------------------------------
/** Is called when this check structure is triggered. This then can cause
 *  a lap to be counted, animation to be started etc.
 */
void CheckStructure::trigger(unsigned int kart_index)
{
    switch(m_check_type)
    {
    case CT_NEW_LAP : World::getWorld()->newLap(kart_index); 
                      m_is_active[kart_index] = false;
                      if(UserConfigParams::m_check_debug)
                      {
                          printf("CHECK: %s new lap %d triggered, now deactivated.\n",
                              World::getWorld()->getKart(kart_index)->getIdent().c_str(),
                              m_index);
                      }
                      break;
    case CT_ACTIVATE: {
                          CheckStructure *cs=
                            m_check_manager->getCheckStructure(m_activate_check_index);
                          cs->m_is_active[kart_index] = true;
                          if(UserConfigParams::m_check_debug)
                          {
                              printf("CHECK: %s %d triggered, activating %d.\n",
                                     World::getWorld()->getKart(kart_index)->getIdent().c_str(),
                                     m_index, m_activate_check_index);
                          }
                          break;
                      }
    case CT_TOGGLE:   {
                          CheckStructure *cs=
                            m_check_manager->getCheckStructure(m_activate_check_index);
                          cs->m_is_active[kart_index] = !cs->m_is_active[kart_index];
                          if(UserConfigParams::m_check_debug)
                          {
                              // At least on gcc 4.3.2 we can't simply print 
                              // cs->m_is_active[kart_index] ("cannot pass objects of
                              // non-POD type ‘struct std::_Bit_reference’ through ‘...’; 
                              // call will abort at runtime"). So we use this somewhat
                              // unusual but portable construct.
                              printf("CHECK: %s %d triggered, setting %d to %d.\n",
                                     World::getWorld()->getKart(kart_index)->getIdent().c_str(),
                                     m_index, m_activate_check_index,
                                     cs->m_is_active[kart_index]==true);
                          }
                          break;
                      }
    default:          break;
    }   // switch m_check_type
}   // trigger
