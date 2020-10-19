idf_component_register(SRCS "ethernet_main.c"
                        "iothub_client_sample_mqtt.c"
                        INCLUDE_DIRS ".")

component_compile_definitions(SET_TRUSTED_CERT_IN_SAMPLES)

