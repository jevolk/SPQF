/** 
 *  COPYRIGHT 2014 (C) Jason Volk
 *  COPYRIGHT 2014 (C) Svetlana Tkachenko
 *
 *  DISTRIBUTED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) (see: LICENSE)
 */


struct Callbacks : irc_callbacks_t
{

    Callbacks();
};


extern Callbacks bot_cbs;       // irclib.cpp
