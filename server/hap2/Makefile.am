SUBDIRS = hatohol
hatohol_hap2dir=$(libexecdir)/$(PACKAGE)/hap2

hatohol_hap2_SCRIPTS = \
	start-stop-hap2-ceilometer.sh \
	start-stop-hap2-fluentd.sh \
	start-stop-hap2-nagios-ndoutils.sh \
	start-stop-hap2-zabbix-api.sh

hatohol_hap2_DATA = \
	hap2-control-functions.sh

noinst_SCRIPTS = \
	setup_common.py \
	setup_zabbixapi.py \
	setup_rabbitmqconnector.py

install-data-hook:
	PYTHON_SETUP_OPTIONS=""; \
	if test -n "$(DESTDIR)"; then \
		PYTHON_SETUP_OPTIONS="--root=$(DESTDIR)"; \
	fi; \
	if test -n "$(prefix)"; then \
		PYTHON_SETUP_OPTIONS="$$PYTHON_SETUP_OPTIONS --prefix=$(prefix)"; \
	fi; \
	python setup_common.py install \
	  $$PYTHON_SETUP_OPTIONS --record common-files.txt && \
	python setup_zabbixapi.py install \
	  $$PYTHON_SETUP_OPTIONS --record zabbixapi-files.txt && \
	python setup_rabbitmqconnector.py install \
	  $$PYTHON_SETUP_OPTIONS --record rabbitmqconnector-files.txt

uninstall-hook:
	cat common-files.txt | xargs rm -f
	cat zabbixapi-files.txt | xargs rm -f
	cat rabbitmqconnector-files.txt | xargs rm -f

nobase_hatohol_hap2_SCRIPTS = \
	hatohol/hap2_zabbix_api.py \
	hatohol/hap2_fluentd.py \
	hatohol/hap2_ceilometer.py \
	hatohol/hap2_nagios_ndoutils.py

EXTRA_DIST = \
	hap2-control-functions.sh.in \
	$(hatohol_hap2_SCRIPTS) \
	$(hatohol_hap2_DATA) \
	$(noinst_SCRIPTS) \
	$(nobase_hatohol_hap2_SCRIPTS)

CLEANFILES = \
	common-files.txt \
	zabbixapi-files.txt \
	rabbitmqconnector-files.txt
