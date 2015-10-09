BUILD_KALIVEDA_MODULE(geometry
   PARENT ${KVSUBPROJECT}
	KVMOD_DEPENDS base events particles stopping
   LIB_EXCLUDE KVASGroup KVLayer KVRing KVTelescope
)
