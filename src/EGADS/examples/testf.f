	program main
c
c      EGADS: Electronic Geometry Aircraft Design System
c
c             Gerenal FORTRAN build test
c
c      Copyright 2011-2022, Massachusetts Institute of Technology
c      Licensed under The GNU Lesser General Public License, version 2.1
c      See http://www.opensource.org/licenses/lgpl-2.1.php
c
c
	include 'egads.inc'
	pointer (pbody, bodies),   (pshell, shells)
	integer*8       bodies(*),          shells(*)
        pointer (psenses, senses)
	integer           senses(*)
	integer*8    context8, model8(2), geom8, topo8(2)
	real*8       limits(4)
	character*80 name

	istat = IG_open(context8)
	write(*,*) 'IG_open = ', istat

	write(*,*) 'Enter Filename:'
	read(*,'(a)') name

	istat = IG_loadModel(context8, 0, name, model8)
	write(*,*) 'IG_loadModel = ', istat

	istat = IG_getTopology(model8, geom8, iclass, mtype, limits,
     &                         nbody, pbody, psenses)
	write(*,*) 'IG_getTopology = ', istat, nbody

	istat = IG_getBodyTopos(bodies(1), NULL, SHELL, nshell, pshell)
	write(*,*) 'IG_getBodyTopos = ', istat, nshell

	istat = IG_makeTopology(context8, NULL, BODY, SOLIDBODY,
     &                          limits, nshell, shells, senses, topo8)
	write(*,*) ' IG_makeTopology = ', istat

	call IG_free(pshell)
	call IG_free(pbody)

	istat = IG_deleteObject(model8)
	write(*,*) 'IG_deleteObject = ', istat

	istat = IG_makeTopology(context8, NULL, MODEL, 0,
     &                          limits, 1, topo8, senses, model8)
	write(*,*) ' IG_makeTopology = ', istat

	istat = IG_saveModel(model8, 'copy.BRep')
	write(*,*) 'IG_saveModel     = ', istat

	istat = IG_deleteObject(model8)
	write(*,*) 'IG_deleteObject = ', istat
c	this gets consumed via the model
C	istat = IG_deleteObject(topo8)
C	write(*,*) 'IG_deleteObject = ', istat

	istat = IG_close(context8)
	write(*,*) 'IG_close = ', istat
c
	call exit
	end
