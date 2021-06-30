#
ifndef ESP_ROOT
$(error ESP_ROOT must be set -- Please fix the environment...)
endif

ifdef ESP_BLOC
TDIR  = $(ESP_BLOC)/examples
COPY  = copy
else
TDIR  = .
COPY  =
endif

all: $(COPY) | $(TDIR) 
	(cd cCAPS; make -f avlTest.make)
	(cd cCAPS; make -f frictionTest.make)
	(cd cCAPS; make -f fun3d.make)
	(cd cCAPS; make -f hsm.make)
	(cd cCAPS; make -f mystran.make)
	(cd cCAPS; make -f aeroelastic_SU2.make)
	(cd cCAPS; make -f pointwiseTest.make)

clean:
	(cd cCAPS; make -f avlTest.make clean)
	(cd cCAPS; make -f frictionTest.make clean)	
	(cd cCAPS; make -f fun3d.make clean)
	(cd cCAPS; make -f hsm.make clean)
	(cd cCAPS; make -f mystran.make clean)
	(cd cCAPS; make -f aeroelastic_SU2.make clean)
	(cd cCAPS; make -f pointwiseTest.make clean)

cleanall:
	(cd cCAPS; make -f avlTest.make cleanall)
	(cd cCAPS; make -f frictionTest.make cleanall)
	(cd cCAPS; make -f fun3d.make cleanall)
	(cd cCAPS; make -f hsm.make cleanall)
	(cd cCAPS; make -f mystran.make cleanall)
	(cd cCAPS; make -f aeroelastic_SU2.make cleanall)
	(cd cCAPS; make -f pointwiseTest.make cleanall)

ifdef ESP_BLOC
endif

ifdef PYTHONINC
test: cCAPStest pyCAPStest
else
test: cCAPStest
endif

cCAPStest: all $(COPY)
	(cd $(TDIR)/regressionTest; ./execute_CTestRegression.sh $(TYPE))

ifdef PYTHONINC
pyCAPStest: all $(COPY)
	(cd $(TDIR)/regressionTest; ./execute_PyTestRegression.sh $(TYPE))
endif

ifdef ESP_BLOC

PYFILES=$(wildcard pyCAPS/*.py)
PYTEST=$(patsubst pyCAPS/%.py,$(TDIR)/pyCAPS/%.py,$(PYFILES))

CSMFILES=$(wildcard csmData/*.csm)
CSMFILES+=$(wildcard csmData/*.udc)
CSMTEST=$(patsubst csmData/%,$(TDIR)/csmData/%,$(CSMFILES))

REGFILES=$(wildcard regressionTest/*.*)
REGTEST=$(patsubst regressionTest/%,$(TDIR)/regressionTest/%,$(REGFILES))

copy: $(TDIR)/csmData $(TDIR)/cCAPS $(TDIR)/pyCAPS $(TDIR)/regressionTest \
	$(PYTEST) $(CSMTEST) $(REGTEST)

$(TDIR):
	mkdir -p $(TDIR)
$(TDIR)/cCAPS: | $(TDIR)
	mkdir -p $(TDIR)/cCAPS
$(TDIR)/pyCAPS: $(TDIR)
	mkdir -p $(TDIR)/pyCAPS
$(TDIR)/csmData: | $(TDIR)
	mkdir -p $(TDIR)/csmData
$(TDIR)/regressionTest: | $(TDIR)
	mkdir -p $(TDIR)/regressionTest
	
$(PYTEST): $(TDIR)/pyCAPS/%.py: pyCAPS/%.py | $(TDIR)/pyCAPS
	cp -p -f $< $@

$(CSMTEST): $(TDIR)/csmData/%: csmData/% | $(TDIR)/csmData
	cp -p -f $< $@

$(REGTEST): $(TDIR)/regressionTest/%: regressionTest/% | $(TDIR)/regressionTest
	cp -p -f $< $@

endif
