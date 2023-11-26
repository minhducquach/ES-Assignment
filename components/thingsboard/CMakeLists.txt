idf_component_register(SRCS "thingsboard.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_client
                    REQUIRES mqtt
                    REQUIRES data_package
                    REQUIRES envi)

