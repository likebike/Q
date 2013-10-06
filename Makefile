# Adjust this Makefile to suit your server and project requirements.

MYROOT=${HOME}/tmp/q_site

# The MakoFW location:
MAKOFW_DIR=${MYROOT}/makofw

# The "source" files:
SRC_DIR=${MYROOT}/src

# The publication locations:
export WWW_ROOT:=${MYROOT}/www
WWW_DEV=${WWW_ROOT}/dev
WWW_PROD=${WWW_ROOT}/prod

# The python binary to use:
PYTHON=python2.7

export PYTHONPATH:=${MAKOFW_DIR}/lib/python:${PYTHONPATH}


# By default, publish to the development site:
dev: touch sync_bigfiles raw_dev
raw_dev:
	${PYTHON} ${MAKOFW_DIR}/bin/publish.py "${SRC_DIR}" "${WWW_DEV}"

# Publish to the development site without doing as many ACL checks:
fast: touch sync_bigfiles raw_fast
raw_fast:
	ACL_CHECK=0 ${PYTHON} ${MAKOFW_DIR}/bin/publish.py "${SRC_DIR}" "${WWW_DEV}"

# Publish to the production site:
prod: touch sync_bigfiles raw_prod
raw_prod:
	${PYTHON} ${MAKOFW_DIR}/bin/publish.py "${SRC_DIR}" "${WWW_PROD}"

sync_bigfiles:
	( cd ${SRC_DIR}/bin; ln -sf ${WWW_ROOT}/bigfiles/* ./ )

# Touch some critical files that force the whole site to be rebuilt:
touch:
	touch ${SRC_DIR}/_master.html.tmpl

