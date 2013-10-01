# Adjust this Makefile to suit your server and project requirements.


# The MakoFW location:
MAKOFW_DIR=${HOME}/makofw

# The "source" files:
SRC_DIR=${HOME}/src

# The publication locations:
WWW_DEV=${HOME}/www/dev
WWW_PROD=${HOME}/www/prod

# The python binary to use:
PYTHON=python2.7


export PYTHONPATH:=${MAKOFW_DIR}/lib/python:${PYTHONPATH}


# By default, publish to the development site:
dev: touch raw_dev
raw_dev:
	${PYTHON} ${MAKOFW_DIR}/bin/publish.py "${SRC_DIR}" "${WWW_DEV}"

# Publish to the development site without doing as many ACL checks:
fast: touch raw_fast
raw_fast:
	ACL_CHECK=0 ${PYTHON} ${MAKOFW_DIR}/bin/publish.py "${SRC_DIR}" "${WWW_DEV}"

# Publish to the production site:
prod: touch raw_prod
raw_prod:
	${PYTHON} ${MAKOFW_DIR}/bin/publish.py "${SRC_DIR}" "${WWW_PROD}"

# Touch some critical files that force the whole site to be rebuilt:
touch:
	touch ${SRC_DIR}/_master.html.tmpl

