#
# Change QUTILSDIR and UTILSDIR to match your setup
#

QUTILSDIR = ../openquartz/bin
UTILSDIR  = ../utils/bin
GAMEDIR   = ../../Evil.activity/id1

QBSP     = ../../tools/hmap/bin/hqbsp
VIS      = ../../tools/hmap/bin/hvis -fast

QCC      = ../utils/qutils/bin/qcc
MODELGEN = $(UTILSDIR)/modelgen
SPRGEN   = $(UTILSDIR)/sprgen
QLUMPY   = $(UTILSDIR)/qlumpy
QFILES   = $(UTILSDIR)/qfiles
QPAKMAN	 = ../../tools/qpakman/qpakman
PAR      = $(UTILSDIR)/par

LIGHT    = ../../tools/hmap/bin/hlight


# top-level targets
ALLPAKS  = pak0.pak

all: $(ALLPAKS)

# installs the compiled files into the game directory
install : all
#	cp $(ALLPAKS) ../openquartz/id1
	cp $(ALLPAKS) $(GAMEDIR)


#bundle	: all
#	cp ../openquartz/openquartz/

extra-maps : \
    maps/dm2ish.bsp maps/dm3ish.bsp maps/dm4ish.bsp     \
    maps/e1m2ish.bsp maps/q3dm6ish.bsp \
    maps/ukpak4.bsp


# be careful, don't erase anything we can't generate
#clean:
	$(RM) $(ALLPAKS)
	$(RM) progs.dat qwprogs.dat qwdefs.h progdefs.h
	$(RM) tyrlite.log
	$(RM) gfx/*.lmp gfx.wad
	$(RM) maps/*.h1 maps/*.h2 maps/*.pts maps/*.prt
	$(RM) $(addprefix maps/,b_batt0.bsp b_batt1.bsp)
	$(RM) $(addprefix maps/,b_bh10.bsp b_bh25.bsp b_bh100.bsp)
	$(RM) $(addprefix maps/,b_explob.bsp b_exbox2.bsp)
	$(RM) $(addprefix maps/,b_nail0.bsp b_nail1.bsp)
	$(RM) $(addprefix maps/,b_rock0.bsp b_rock1.bsp)
	$(RM) $(addprefix maps/,b_shell0.bsp b_shell1.bsp)
	$(RM) $(addprefix maps/,void4.bsp void5.bsp void6.bsp)
	$(RM) $(addprefix maps/,vtest.bsp box.bsp am1.bsp)
	$(RM) $(addprefix maps/,bunkers.bsp house.bsp nsa.bsp)
	$(RM) $(addprefix maps/,office.bsp start.bsp q3dm6ish.bsp)
	$(RM) $(addprefix maps/,elbow.bsp devourer.bsp rpgdm1.bsp)
	$(RM) $(addprefix maps/,d1.bsp d2.bsp d3.bsp d4.bsp)
	$(RM) $(addprefix maps/,d5.bsp d6.bsp d7.bsp d8.bsp)
	$(RM) $(addprefix maps/,q1start.bsp)
	$(RM) $(addprefix maps/,e1m1.bsp e1m2.bsp e1m3.bsp e1m4.bsp)
	$(RM) $(addprefix maps/,e1m5.bsp e1m6.bsp e1m7.bsp e1m8.bsp)
	$(RM) $(addprefix maps/,e2m1.bsp e2m2.bsp e2m3.bsp e2m4.bsp)
	$(RM) $(addprefix maps/,e2m5.bsp e2m6.bsp e2m7.bsp)
	$(RM) $(addprefix maps/,e3m1.bsp e3m2.bsp e3m3.bsp e3m4.bsp)
	$(RM) $(addprefix maps/,e3m5.bsp e3m6.bsp e3m7.bsp)
	$(RM) $(addprefix maps/,e4m1.bsp e4m2.bsp e4m3.bsp e4m4.bsp)
	$(RM) $(addprefix maps/,e4m5.bsp e4m6.bsp e4m7.bsp e4m8.bsp)
	$(RM) $(addprefix maps/,end.bsp)
	$(RM) $(addprefix maps/,e2m10.bsp)
	$(RM) $(addprefix progs/,b_g_key.mdl m_g_key.mdl w_g_key.mdl)
	$(RM) $(addprefix progs/,b_s_key.mdl m_s_key.mdl w_s_key.mdl)
	$(RM) $(addprefix progs/,end1.mdl end2.mdl end3.mdl end4.mdl)
	$(RM) $(addprefix progs/,gib1.mdl gib2.mdl gib3.mdl zom_gib.mdl)
	$(RM) $(addprefix progs/,eyes.mdl flame.mdl flame2.mdl lavaball.mdl)
	$(RM) $(addprefix progs/,laser.mdl armor.mdl backpack.mdl teleport.mdl)
	$(RM) $(addprefix progs/,quaddama.mdl invisibl.mdl invulner.mdl suit.mdl)
	$(RM) $(addprefix progs/,g_light.mdl bolt.mdl bolt2.mdl bolt3.mdl)
	$(RM) $(addprefix progs/,g_nail.mdl g_nail2.mdl s_spike.mdl spike.mdl)
	$(RM) $(addprefix progs/,g_rock.mdl g_rock2.mdl grenade.mdl missile.mdl)
	$(RM) $(addprefix progs/,g_shot.mdl)
	$(RM) $(addprefix progs/,h_player.mdl)
	$(RM) $(addprefix progs/,h_dog.mdl h_guard.mdl h_knight.mdl h_zombie.mdl)
	$(RM) $(addprefix progs/,wizard.mdl h_wizard.mdl)
	$(RM) $(addprefix progs/,s_bubble.spr s_explod.spr s_light.spr)

#
# PAK0_CONTENTS is what goes into pak0.pak
#
# It contains the "core" content--content that is shared between single-player
# and multi-player versions of the game
#
PAK0_CONTENTS += default.cfg
PAK0_CONTENTS += end.bin
PAK0_CONTENTS += end2.bin
PAK0_CONTENTS += gfx.wad
PAK0_CONTENTS += gfx/bigbox.lmp
PAK0_CONTENTS += gfx/box_bl.lmp
PAK0_CONTENTS += gfx/box_bm.lmp
PAK0_CONTENTS += gfx/box_br.lmp
PAK0_CONTENTS += gfx/box_ml.lmp
PAK0_CONTENTS += gfx/box_mm.lmp
PAK0_CONTENTS += gfx/box_mm2.lmp
PAK0_CONTENTS += gfx/box_mr.lmp
PAK0_CONTENTS += gfx/box_tl.lmp
PAK0_CONTENTS += gfx/box_tm.lmp
PAK0_CONTENTS += gfx/box_tr.lmp
PAK0_CONTENTS += gfx/colormap.lmp
PAK0_CONTENTS += gfx/complete.lmp
PAK0_CONTENTS += gfx/conback.lmp
PAK0_CONTENTS += gfx/dim_drct.lmp
PAK0_CONTENTS += gfx/dim_ipx.lmp
PAK0_CONTENTS += gfx/dim_modm.lmp
PAK0_CONTENTS += gfx/dim_mult.lmp
PAK0_CONTENTS += gfx/dim_tcp.lmp
PAK0_CONTENTS += gfx/finale.lmp
PAK0_CONTENTS += gfx/help0.lmp
PAK0_CONTENTS += gfx/help1.lmp
PAK0_CONTENTS += gfx/help2.lmp
PAK0_CONTENTS += gfx/help3.lmp
PAK0_CONTENTS += gfx/help4.lmp
PAK0_CONTENTS += gfx/help5.lmp
PAK0_CONTENTS += gfx/inter.lmp
PAK0_CONTENTS += gfx/loading.lmp
PAK0_CONTENTS += gfx/mainmenu.lmp
PAK0_CONTENTS += gfx/menudot1.lmp
PAK0_CONTENTS += gfx/menudot2.lmp
PAK0_CONTENTS += gfx/menudot3.lmp
PAK0_CONTENTS += gfx/menudot4.lmp
PAK0_CONTENTS += gfx/menudot5.lmp
PAK0_CONTENTS += gfx/menudot6.lmp
PAK0_CONTENTS += gfx/menuplyr.lmp
PAK0_CONTENTS += gfx/mp_menu.lmp
PAK0_CONTENTS += gfx/netmen1.lmp
PAK0_CONTENTS += gfx/netmen2.lmp
PAK0_CONTENTS += gfx/netmen3.lmp
PAK0_CONTENTS += gfx/netmen4.lmp
PAK0_CONTENTS += gfx/netmen5.lmp
PAK0_CONTENTS += gfx/p_load.lmp
PAK0_CONTENTS += gfx/p_multi.lmp
PAK0_CONTENTS += gfx/p_option.lmp
PAK0_CONTENTS += gfx/p_save.lmp
PAK0_CONTENTS += gfx/palette.lmp
PAK0_CONTENTS += gfx/pause.lmp
PAK0_CONTENTS += gfx/pop.lmp
PAK0_CONTENTS += gfx/qplaque.lmp
PAK0_CONTENTS += gfx/ranking.lmp
PAK0_CONTENTS += gfx/sell.lmp
PAK0_CONTENTS += gfx/sp_menu.lmp
PAK0_CONTENTS += gfx/ttl_cstm.lmp
PAK0_CONTENTS += gfx/ttl_main.lmp
PAK0_CONTENTS += gfx/ttl_sgl.lmp
PAK0_CONTENTS += gfx/vidmodes.lmp
PAK0_CONTENTS += maps/b_batt0.bsp
PAK0_CONTENTS += maps/b_batt1.bsp
PAK0_CONTENTS += maps/b_bh10.bsp
PAK0_CONTENTS += maps/b_bh100.bsp
PAK0_CONTENTS += maps/b_bh25.bsp
PAK0_CONTENTS += maps/b_exbox2.bsp
PAK0_CONTENTS += maps/b_explob.bsp
PAK0_CONTENTS += maps/b_nail0.bsp
PAK0_CONTENTS += maps/b_nail1.bsp
PAK0_CONTENTS += maps/b_rock0.bsp
PAK0_CONTENTS += maps/b_rock1.bsp
PAK0_CONTENTS += maps/b_shell0.bsp
PAK0_CONTENTS += maps/b_shell1.bsp
PAK0_CONTENTS += maps/b_shell2.bsp
PAK0_CONTENTS += progs.dat
PAK3_CONTENTS += maps/start.lights
PAK0_CONTENTS += progs/armor.mdl
PAK0_CONTENTS += progs/misc/mine1.mdl
PAK0_CONTENTS += progs/misc/mine2.mdl
PAK0_CONTENTS += progs/misc/mine3.mdl
PAK0_CONTENTS += progs/misc/mine4.mdl
PAK0_CONTENTS += progs/misc/mine5.mdl
PAK0_CONTENTS += progs/misc/mine6.mdl
PAK0_CONTENTS += progs/misc/mine7.mdl
PAK0_CONTENTS += progs/misc/mine8.mdl
PAK0_CONTENTS += progs/misc/mine9.mdl
PAK0_CONTENTS += progs/misc/mine10.mdl
PAK0_CONTENTS += progs/misc/diamond.mdl
PAK0_CONTENTS += progs/b_g_key.mdl
PAK0_CONTENTS += progs/b_s_key.mdl
PAK0_CONTENTS += progs/backpack.mdl
PAK0_CONTENTS += progs/bolt.mdl
PAK0_CONTENTS += progs/bolt2.mdl
PAK0_CONTENTS += progs/bolt3.mdl
PAK0_CONTENTS += progs/end1.mdl
PAK0_CONTENTS += progs/end2.mdl
PAK0_CONTENTS += progs/end3.mdl
PAK0_CONTENTS += progs/end4.mdl
PAK0_CONTENTS += progs/eyes.mdl
PAK0_CONTENTS += progs/flame.mdl
PAK0_CONTENTS += progs/flame2.mdl
PAK0_CONTENTS += progs/g_light.mdl
PAK0_CONTENTS += progs/g_nail.mdl
PAK0_CONTENTS += progs/g_nail2.mdl
PAK0_CONTENTS += progs/g_rock.mdl
PAK0_CONTENTS += progs/g_rock2.mdl
PAK0_CONTENTS += progs/g_shot.mdl
PAK0_CONTENTS += progs/gib1.mdl
PAK0_CONTENTS += progs/gib2.mdl
PAK0_CONTENTS += progs/gib3.mdl
PAK0_CONTENTS += progs/grenade.mdl
PAK0_CONTENTS += progs/h_player.mdl
PAK0_CONTENTS += progs/h_zombie.mdl
PAK0_CONTENTS += progs/invisibl.mdl
PAK0_CONTENTS += progs/invulner.mdl
PAK0_CONTENTS += progs/laser.mdl
PAK0_CONTENTS += progs/lavaball.mdl
PAK0_CONTENTS += progs/m_g_key.mdl
PAK0_CONTENTS += progs/m_s_key.mdl
PAK0_CONTENTS += progs/missile.mdl
PAK0_CONTENTS += progs/player.mdl
PAK0_CONTENTS += progs/quaddama.mdl
PAK0_CONTENTS += progs/s_bubble.spr
PAK0_CONTENTS += progs/s_explod.spr
PAK0_CONTENTS += progs/s_invis.spr
PAK0_CONTENTS += progs/s_light.spr
PAK0_CONTENTS += progs/s_spike.mdl
PAK0_CONTENTS += progs/spike.mdl
PAK0_CONTENTS += progs/suit.mdl
PAK0_CONTENTS += progs/teleport.mdl
PAK0_CONTENTS += progs/v_axe.mdl
PAK0_CONTENTS += progs/v_light.mdl
PAK0_CONTENTS += progs/v_nail.mdl
PAK0_CONTENTS += progs/v_nail2.mdl
PAK0_CONTENTS += progs/v_rock.mdl
PAK0_CONTENTS += progs/v_rock2.mdl
PAK0_CONTENTS += progs/v_shot.mdl
PAK0_CONTENTS += progs/v_shot2.mdl
PAK0_CONTENTS += progs/w_g_key.mdl
PAK0_CONTENTS += progs/w_s_key.mdl
PAK0_CONTENTS += progs/zom_gib.mdl
PAK0_CONTENTS += progs/zombie.mdl
PAK0_CONTENTS += quake.rc
#PAK0_CONTENTS += qwprogs.dat


#
# PAK0_CONTENTS is what goes into pak1.pak
#
# It contains models for the multiplayer game.
#

PAK0_CONTENTS += progs/h_player.mdl

PAK0_CONTENTS += progs/s_light.mdl
PAK0_CONTENTS += progs/s_spike.mdl


#
# PAK2_CONTENTS is what goes into pak2.pak
#
# It contains sounds.
#
PAK0_CONTENTS += sound/misc/fire_ambient.wav
PAK0_CONTENTS += sound/ambience/fire1.wav
PAK0_CONTENTS += sound/ambience/water1.wav
PAK0_CONTENTS += sound/ambience/wind2.wav
PAK0_CONTENTS += sound/ambience/windfly.wav
PAK0_CONTENTS += sound/buttons/airbut1.wav
PAK0_CONTENTS += sound/buttons/switch02.wav
PAK0_CONTENTS += sound/buttons/switch04.wav
PAK0_CONTENTS += sound/buttons/switch21.wav
PAK0_CONTENTS += sound/doors/basetry.wav
PAK0_CONTENTS += sound/doors/baseuse.wav
PAK0_CONTENTS += sound/doors/ddoor2.wav
PAK0_CONTENTS += sound/doors/doormv1.wav
PAK0_CONTENTS += sound/doors/drclos4.wav
PAK0_CONTENTS += sound/doors/stndr1.wav
PAK0_CONTENTS += sound/doors/stndr2.wav
PAK0_CONTENTS += sound/items/damage.wav
PAK0_CONTENTS += sound/items/damage2.wav
PAK0_CONTENTS += sound/items/damage3.wav
PAK0_CONTENTS += sound/misc/null.wav
PAK0_CONTENTS += sound/player/death1.wav
PAK0_CONTENTS += sound/player/death2.wav
PAK0_CONTENTS += sound/player/death3.wav
PAK0_CONTENTS += sound/player/death4.wav
PAK0_CONTENTS += sound/player/death5.wav
PAK0_CONTENTS += sound/player/drown1.wav
PAK0_CONTENTS += sound/player/drown2.wav
PAK0_CONTENTS += sound/player/gasp1.wav
PAK0_CONTENTS += sound/player/gasp2.wav
PAK0_CONTENTS += sound/player/gib.wav



#
# PAK3_CONTENTS is what goes into pak3.pak
#
# It contains multiplayer maps.
#
#
#
PAK0_CONTENTS += maps/start.bsp
#PAK3_CONTENTS += maps/level1.bsp
#PAK3_CONTENTS += maps/level2.bsp
#
# make targets
#

pak0.pak: $(PAK0_CONTENTS) makefile
	$(PAR) -c $@ $(PAK0_CONTENTS)

progs.dat:
	$(QCC) -o progs.dat -p progdefs.h qc/progs.c

gfx.wad :
	$(QLUMPY) gfx_wad.ls

gfx :
	$(QPAKMAN) gfx/*.png -g quake1 -o gfx.wad

textures :
	$(QPAKMAN) maps/textures/*.png -g quake1 -o maps/textures/free_wad.wad
	cp maps/textures/free_wad.wad $(GAMEDIR)/free_wad.wad


%.lmp : gfx_lmp.ls
	$(QLUMPY) gfx_lmp.ls

#%.mdl : %.qc
#	$(MODELGEN) $<

%.spr : %.qc %.png
	$(SPRGEN) $<

maps/start.lights  : ;
#
# Static models are models that don't move during the game, so the 
# can be represented by a .bsp instead of a .mdl (which is easier to render).
# Unlike regular .map files, $(VIS) cannot be run on them
# Static models are models that don't move during the game, so the 
# can be represented by a .bsp instead of a .mdl (which is easier to render).
# Unlike regular .map files, $(VIS) cannot be run on them
# because they are inherently leaky.  Therefore, we have to add
# a target to that matches them specially.  This MUST come before the
# normal rules for building maps so that it will match first.
#
maps/b_%.bsp : maps/b_%.map
	$(QBSP) $<
	$(LIGHT) $@

%.bsp : %.map
	$(QBSP) $<
	$(VIS) $@
	$(LIGHT) $@

progs/s_light.mdl : ;

