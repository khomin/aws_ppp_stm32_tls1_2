/*
 * settings.c
 *
 *  Created on: Jul 6, 2019
 *      Author: khomin
 */
#include "settings/settings.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdio.h>
#include "debug_print.h"

//#define USE_OLD_CONF_CERTIFICATES
//#define USE_BILLS

//-- root CA
uint8_t __attribute__((section (".SettingsData"))) CLIENT_ROOT_CA[USER_CONF_TLS_ROOT_CA_CERT] =
		"-----BEGIN CERTIFICATE-----\r\n" \
		"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n" \
		"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n" \
		"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n" \
		"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n" \
		"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n" \
		"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n" \
		"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n" \
		"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n" \
		"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n" \
		"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n" \
		"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n" \
		"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n" \
		"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n" \
		"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n" \
		"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n" \
		"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n" \
		"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n" \
		"rqXRfboQnoZsG4q5WTP468SQvvG5\r\n" \
		"-----END CERTIFICATE-----\r\n";

//-- device certificate
#ifdef USE_OLD_CONF_CERTIFICATES
uint8_t __attribute__((section (".SettingsData"))) CLIENT_PRIVATE_DEVICE_CERT[USER_CONF_TLS_OBJECT_MAX_SIZE] =
		"-----BEGIN CERTIFICATE-----\r\n" \
		"MIIDWjCCAkKgAwIBAgIVAK8Wxv0/RRakxZ1Fq9FqYCtcvxQ9MA0GCSqGSIb3DQEB\r\n" \
		"CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\r\n" \
		"IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0xOTA2MDEyMzA1\r\n" \
		"MTVaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\r\n" \
		"dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDCujKA9v4Ouo5OtsUa\r\n" \
		"/1bcDRubjW5cPZaMNXXe9RWxjXG8E5+YkSDEDEo30PLC442iKYGVLKrrsd0YxjtY\r\n" \
		"OASMqDyUjyUn8Jm4wU7i+6hny7JBIhODUOFmzQyszu01x8glpGo1NDf3UwiykiOZ\r\n" \
		"yofE1hgzFoAHHJFpT3MOYSl2X9rN1wyK/BeOvHJYRCCRsGj4eYCjlIrZIF7fbeyf\r\n" \
		"+ezJ9UDYRntq9b2axncYbRiUVlAJZJPXHt/9Jx2mxZnL+GqWIhtgnE3B+wuapE4K\r\n" \
		"OdYgjf1Er27jrxIxaLeUtd+H3apZSQkZML9dzLo1zHH+kSxNsbRRpxNkNQsj/tZW\r\n" \
		"PqwjAgMBAAGjYDBeMB8GA1UdIwQYMBaAFJRtBk1FKQv216Tb8ZT5POFC42pMMB0G\r\n" \
		"A1UdDgQWBBStDghpZ4+y4JFAGRJzKq35Uq67ZzAMBgNVHRMBAf8EAjAAMA4GA1Ud\r\n" \
		"DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEANQUSEtbdM5Rt5m013rJ4mD4L\r\n" \
		"2XxQmLXt0TPHRe5IMm0s0qQ5n3zQv9rn1BXSLtrPwVA4wjAQngPdk0j3hq1J2nGG\r\n" \
		"T4f1yLaxDT+XdUAl0pQ3hJxpkoOwIHWvILu/usSfXGVWgYD1u3repBIGltqMNF7J\r\n" \
		"BBkTq8ittxcCwHhjHyAvuVTOxal6qbjCipmcPV85xnaY1wj9h+JTw3ZZ8qiGYmfn\r\n" \
		"ksbSaqkTE7P+T5KbEVaO5MyutadXtCU6OvgN0DANUmqt3M9kFl+CBLg1jgc/bq28\r\n" \
		"u9ypwNFLq7MbXIKZ0lO+qZXVgufOSnKmaAXV58jAolMljegVeWd15xtEx3vVGg==\r\n" \
		"-----END CERTIFICATE-----\r\n";
#else
uint8_t __attribute__((section (".SettingsData"))) CLIENT_PRIVATE_DEVICE_CERT[USER_CONF_TLS_OBJECT_MAX_SIZE] =
		"-----BEGIN CERTIFICATE-----\r\n" \
		"MIIDPDCCAiQCCQCgOhOgS4gRDTANBgkqhkiG9w0BAQsFADBgMQswCQYDVQQGEwJS\r\n" \
		"VTEMMAoGA1UECAwDU3BiMQwwCgYDVQQHDANTcGIxETAPBgNVBAoMCEJlbGxzb2Z0\r\n" \
		"MQwwCgYDVQQLDANEZXYxFDASBgNVBAMMC2JlbGwtc3cuY29tMB4XDTE5MDYyNzEw\r\n" \
		"NTczOVoXDTIwMDYyNjEwNTczOVowYDELMAkGA1UEBhMCUlUxDDAKBgNVBAgMA1Nw\r\n" \
		"YjEMMAoGA1UEBwwDU3BiMREwDwYDVQQKDAhCZWxsc29mdDEMMAoGA1UECwwDRGV2\r\n" \
		"MRQwEgYDVQQDDAtiZWxsLXN3LmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\r\n" \
		"AQoCggEBAMjMo7ScqN+ehLun+4F5msh8KZ6YEH04ou4hlV488TgojJNMvEhToalY\r\n" \
		"/m8xPTwZBLlFkYue/7OTu1oXHQpYSIwtqn2KTcctvttUh6rD89biprxVGItCe9t/\r\n" \
		"8VCj3o3+m7Rrd1toZLjaSR8cMUkJ5qk7+1kEV8zqdUU/iZ5qXwK63BYHv82P68uM\r\n" \
		"9Xfn+5HybFD5KdBi5HgnhrVzl2+Ctu7c5nxJqFD3aGq/SuD2Gmd3NbS4F/fEhd7N\r\n" \
		"uF4JtdJhZniju+OdvuHrL2wF59J+Qi5FLniE6K4WkAAmmgJQN40novAR4QW+O+og\r\n" \
		"rV6j5zxhYs9snOsWa5SDRaGLlHj7DTkCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEA\r\n" \
		"NTpfRta92S171U3EhyMzDW53lwchKRyIbr4Zgi+rpPq+Yb8U3+WE6OhfuFu9/b8z\r\n" \
		"eTzewpR2OBK9dO2Ae+pIp5mKpRB+AUrzknwZccFtM6QgsrczaMaRiNSDqkm5F6Ip\r\n" \
		"WQGzmEV1JBD50MSZdT19nJGBRmT6VPgwDBdv+xds+3VCNEgadNVikWC4m4hCs5jS\r\n" \
		"CbbGLpoGVp/NHNtHbhXBOxDpN4BMm49s02iMdV2Z+4fgJudmUa6jdqhTda3gxLt7\r\n" \
		"yK3LBHbz+nmS/Vb1OtH8HlykqcRWdx4+TeI3kJsCvqxk+u3XVyJNRJliZ2Ls2pv2\r\n" \
		"rtMf+pNtlW6bM/wsUGlHhw==\r\n" \
		"-----END CERTIFICATE-----\r\n";
#endif

//-- device key
#ifdef USE_OLD_CONF_CERTIFICATES
uint8_t __attribute__((section (".SettingsData"))) CLIENT_PRIVATE_KEY[USER_CONF_TLS_OBJECT_MAX_SIZE] =
		"-----BEGIN RSA PRIVATE KEY-----\r\n" \
		"MIIEowIBAAKCAQEAwroygPb+DrqOTrbFGv9W3A0bm41uXD2WjDV13vUVsY1xvBOf\r\n" \
		"mJEgxAxKN9DywuONoimBlSyq67HdGMY7WDgEjKg8lI8lJ/CZuMFO4vuoZ8uyQSIT\r\n" \
		"g1DhZs0MrM7tNcfIJaRqNTQ391MIspIjmcqHxNYYMxaABxyRaU9zDmEpdl/azdcM\r\n" \
		"ivwXjrxyWEQgkbBo+HmAo5SK2SBe323sn/nsyfVA2EZ7avW9msZ3GG0YlFZQCWST\r\n" \
		"1x7f/ScdpsWZy/hqliIbYJxNwfsLmqROCjnWII39RK9u468SMWi3lLXfh92qWUkJ\r\n" \
		"GTC/Xcy6Ncxx/pEsTbG0UacTZDULI/7WVj6sIwIDAQABAoIBAGZCQaNYEbLKZVEQ\r\n" \
		"yhGTZZnJiGX3r6zg6t5WdL/RFMNY9BFAfN8x13McwRzurY2K0iYGhfZArsF3dhQ8\r\n" \
		"hY2eXUVAVA/v3CNNRmyvly272oQnhESQEI81OikyJSXaxNcpQwIBhYTLF1jaUgb0\r\n" \
		"l517rjC//ZVWi4Uwi2wyPMmgXJW8djivNAlJvynkk4IU3jvBdT8Hk++5J2+pelcu\r\n" \
		"ChJ3uA6HM361/OkJjaH/XB1+XBijJICGkpFSTxSp8gVxRiKoG28yPKh4deDvaykk\r\n" \
		"r4yLIvHizEXsTOeDIDUEbUczvJLEhrjrTa0wCdhUCBIu0aEi4sg+CzJrEqKKFWgZ\r\n" \
		"e25c2IECgYEA4S6qpdaFOFRRf+6DfXW4lieMHkvwTpf0BERsH83G5EAxz5cSeGyE\r\n" \
		"2oG1coO0Zk4rN0o+gaBpE1C/Qm7BzLwSG9NKGURQScfFXd95tJEMNdEdXG87aY1v\r\n" \
		"MsqwZL9QvuSMFA4VMhTK1tXjMbe9iSBVCKRUF7ku82pwUxMEa8nKVpcCgYEA3WCI\r\n" \
		"ERIGspIagzRAsb5OVusiUtDwWfFOyzAwU025FZVQOK3K9FzFdOx1XG/eO81+Txvl\r\n" \
		"x8VfReW0GEca7DbzDnBiVXXB+JnjOw56RkXsLA4tZhhGiUuoMN2UuJ+XzK82Qx8m\r\n" \
		"7/Jxo3C4YJOMcQVbaTPF9jXCeKi+eJ2K35m49FUCgYALUIn4jT/iZbI2qGho/GpK\r\n" \
		"+S+8MfqgKFh1gm6gZnmQ7HzUPnYKIQHH7O6OE3oUDedz/sIHR2UgOFVz5BbOz8Bl\r\n" \
		"L4ewn1MdcfFD1z/JR+SCK8C67Ufh9N5agf69ATPwc5FqTI7aFlte0h55WA8o/bvo\r\n" \
		"FEG5c2+dgYKlWfZF6DzHfwKBgFKWDYzYvbjmeo+R8cSda1pPs+lTWycit7NjkdOx\r\n" \
		"/idIIbpTU5Cf/2XkRNAsR6rluYZHsQw63JUV4X7hH/EZeslbMcQR/0AAIN5KQ2Ft\r\n" \
		"+nJhA5y/16immeNI9o99skLA5qxZtcv0FWLUzWPyB4WuavCvSN3WJmufABe9Ji7O\r\n" \
		"iUpVAoGBAL6KLIQWBEwXYTMhGu8r9kfeRyUbsKdr1Zx6gNV3d6A6gGHUBVUblPvg\r\n" \
		"KB41gbcmXPpg/C61SOjOMEcLlYFGpLNLyYt9TB+15oQh3t8PVuVTSBqFu7tSr0DJ\r\n" \
		"8/1n9peHWl01Om/PMjv0PxDl3urVlVPo+ZRdga/ATZL3geDEZCCj\r\n" \
		"-----END RSA PRIVATE KEY-----\r\n";
#else
uint8_t __attribute__((section (".SettingsData"))) CLIENT_PRIVATE_KEY[USER_CONF_TLS_OBJECT_MAX_SIZE] =
		"-----BEGIN RSA PRIVATE KEY-----\r\n" \
		"MIIEpAIBAAKCAQEAyMyjtJyo356Eu6f7gXmayHwpnpgQfTii7iGVXjzxOCiMk0y8\r\n" \
		"SFOhqVj+bzE9PBkEuUWRi57/s5O7WhcdClhIjC2qfYpNxy2+21SHqsPz1uKmvFUY\r\n" \
		"i0J723/xUKPejf6btGt3W2hkuNpJHxwxSQnmqTv7WQRXzOp1RT+JnmpfArrcFge/\r\n" \
		"zY/ry4z1d+f7kfJsUPkp0GLkeCeGtXOXb4K27tzmfEmoUPdoar9K4PYaZ3c1tLgX\r\n" \
		"98SF3s24Xgm10mFmeKO7452+4esvbAXn0n5CLkUueITorhaQACaaAlA3jSei8BHh\r\n" \
		"Bb476iCtXqPnPGFiz2yc6xZrlINFoYuUePsNOQIDAQABAoIBAHj2jkfnd/P+UoeZ\r\n" \
		"knFVCGFuKsRXZteInt5FbO3wWIA0LTsvJt2LQ//4lI33Y6QojifuONebWP6dKGgF\r\n" \
		"NIFe3ZVUjThDcMdkT21hZrkAgowYzcj2mmqKCoMYeA7UKOXxU2tEsgpmwQZ6uUH8\r\n" \
		"gdQ2GrYoZCoj66COPUcSF51PBx1k0cR5ffWcVuyeBpSKJK1gc3RhhCP9UpYZaDQ5\r\n" \
		"AeJRG/xs/IvJG8voMYq3kW3W7CjVFwvGQ8hYA1J9p1MSNfPnTHMk5m5iOAHkyWwZ\r\n" \
		"UMEegO0l9f9hFY67qLodzttRsKF1yY2Q6SerMn25DNwCSGgIBXMNldaex2051Pm1\r\n" \
		"sBCkUAECgYEA9lxIx3aVose7BFEeuhecqnXe6Ss+bMC3+9EopeR0gM7J/et+0Qx4\r\n" \
		"P4o65teCycPraERECKr4uaP1rfFmu4d8TiTlAaoX/ruvOEe95Q5lymGX3hJbG/7c\r\n" \
		"a5JXlxxLh5UdxAP8V2umqBaYgmrDTa6eiowbDiRUmhgk8ikjjjDep/kCgYEA0Kf7\r\n" \
		"2tQ9RwmZESQwEBi53pAcoqbljaMgfzxDiatNTTmxiLJeyg8XlUaL/UplFqUYH5pI\r\n" \
		"EruTOx77Jdx3wnzra+GCSJk5+PocJRi/V53edYuU9u7bslVfcYdXH5Ply+ikYo6/\r\n" \
		"BPSVq4bj9/qtrrQX72EF3gNeSLgRVIlnY/pFX0ECgYEAvXpKy4ZsHg9pyi8t58ij\r\n" \
		"SQNxF3qX/4LVvoqmrbig1jS1bGMEXXouvgftt3/HarERzFa80MHWnMj6+vZwogjZ\r\n" \
		"VSzRKU+ONvBZGnsrFRHWvioDaNxLUKPbGa0rSuCLQtMwWoMKQJ5eRmdikuPUJFoK\r\n" \
		"O9r4MA9HNmEdgVacrw7tINkCgYAPj/fg4mOPoM+hz6kmCjISv3zjRL9qSPhrhPsk\r\n" \
		"kXo6gEsVfvhC6c1AfPqD8cCIZ9fcw40MmKDqj+z7be4gu2Bzs2YcNBF22HCw85+R\r\n" \
		"0Vx+N+LsZ/RK4MM1nHqLCGTjSH66OxtUK0neeTkXjcLWfOgsGnhtjqLBmbjxLS5g\r\n" \
		"7pGuAQKBgQCY1QEVX3HaP3ROczWFEa4ZSaG+4urOjsliLNe0IK49D6gXCKuWh/Y6\r\n" \
		"cjzk2KFeeAR+mNDDRiuj2DOtN9qimYLfcHtHfazn7SPANukDE3R56xt58QWLgS26\r\n" \
		"uDpNJu9BIADbRlIbXa5HvLbqJZOGtgvYkXk03bCYdMlZmjNI/ZFmuQ==\r\n" \
		"-----END RSA PRIVATE KEY-----\r\n";
#endif

//-- connect properies
#ifdef USE_OLD_CONF_CERTIFICATES
//-- mqtt topic - example: a39gt7zyg3mya3-ats.iot.us-east-2.amazonaws.com
uint8_t __attribute__((section (".SettingsData"))) mqttDestEndpoint[USER_CONF_SERVER_NAME_LENGTH] = "a39gt7zyg3mya3-ats.iot.us-east-2.amazonaws.com";
//-- device topic
uint8_t __attribute__((section (".SettingsData"))) mqttTopicPath[USER_CONF_DEVICE_NAME_LENGTH] = "$aws/things/USB_Printer_Board/shadow/update";
//-- device name
uint8_t __attribute__((section (".SettingsData"))) mqttDeviceName[USER_CONF_DEVICE_NAME_LENGTH] = "USB_Printer_Board";
//-- reserv
uint8_t __attribute__((section (".SettingsData"))) reserv[0xFFFFF] = {0};
#else
//-- mqtt topic - example: a39gt7zyg3mya3-ats.iot.us-east-2.amazonaws.com
uint8_t __attribute__((section (".SettingsData"))) mqttDestEndpoint[USER_CONF_SERVER_NAME_LENGTH] = "ajdhfkws66ilz-ats.iot.us-east-2.amazonaws.com";
//-- device name
uint8_t __attribute__((section (".SettingsData"))) mqttDeviceName[USER_CONF_DEVICE_NAME_LENGTH] = "USB_Printer_Board";
//-- device topic
uint8_t __attribute__((section (".SettingsData"))) mqttTopicPath[USER_CONF_DEVICE_NAME_LENGTH] = "bills";
//-- reserv
uint8_t __attribute__((section (".SettingsData"))) reserv[0xFFFFF] = {0};
#endif

/* to workaround a limitation of SFMI tools which support a single section      */
/* so do not mark structure as _no_init                                         */
user_config_t __uninited_region_start__ __attribute__((section("UNINIT_FIXED_LOC")));
const  user_config_t lUserConfigPtr = {
		CLIENT_ROOT_CA,	// tls_root_ca_cert
		CLIENT_PRIVATE_DEVICE_CERT,	// tls_device_cert
		CLIENT_PRIVATE_KEY,	// tls_device_key
		{
				// iot_config
				USER_CONF_MAGIC,
				mqttTopicPath,
				mqttDestEndpoint,
		},
		USER_CONF_MAGIC,	// ca_tls_magic
		USER_CONF_MAGIC // device_tls_magic
};

static const char not_found_caption[] = "not found";

static uint32_t GetSector(uint32_t Address);

void settingsInit() {
	DBGLog("settings: init: status certificates - %d %d %d",
			getKeyCLIENT_CERTIFICATE_PEM_IsExist(),
			getKeyCLIENT_PRIVATE_KEY_PEM_IsExist(),
			getKeyCLIENT_PRIVATE_DEVICE_CERT_PEM_IsExist()
	);
}

bool getKeyCLIENT_CERTIFICATE_PEM_IsExist() {
	return CLIENT_ROOT_CA[0] != 0xFF;
}

bool getKeyCLIENT_PRIVATE_KEY_PEM_IsExist() {
	return CLIENT_PRIVATE_KEY[0] != 0xFF;
}

bool getKeyCLIENT_PRIVATE_DEVICE_CERT_PEM_IsExist() {
	return CLIENT_PRIVATE_DEVICE_CERT[0] != 0xFF;
}

bool getMqttDestEndpoint_isExist() {
	return  mqttDestEndpoint[0] != 0xFF;

}

bool getTopicPath_isExist() {
	return 	mqttTopicPath[0] != 0xFF;
}

bool getDeviceName_isExist() {
	return mqttDeviceName[0] != 0xFF;
}

/*
 * PEM-encoded client certificate
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
bool setKeyCLIENT_CERTIFICATE_PEM(uint8_t *pdata, uint16_t len) {
	HAL_FLASH_Unlock();
	uint32_t addr = (uint32_t)&CLIENT_ROOT_CA;
	uint16_t i = 0;
	for(i=0; i<len; i++) {
		if(HAL_OK != HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr + i, pdata[i])) {
			HAL_FLASH_Lock();
			return false;
		}
	}
	HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr+i, 0);
	HAL_FLASH_Lock();
	return true;
}

/*
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
bool setKeyCLIENT_PRIVATE_KEY_PEM(uint8_t *pdata, uint16_t len) {
	uint16_t i = 0;
	HAL_FLASH_Unlock();
	uint32_t addr = (uint32_t)(&CLIENT_PRIVATE_KEY);
	for(i=0; i<len; i++) {
		if(HAL_OK != HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr + i, pdata[i])) {
			HAL_FLASH_Lock();
			return false;
		}
	}
	HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr+i, 0);
	HAL_FLASH_Lock();
	return true;
}

/*
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
bool setKeyCLIENT_PRIVATE_DEVICE_CERT(uint8_t *pdata, uint16_t len) {
	uint16_t i=0;
	HAL_FLASH_Unlock();
	uint32_t addr = (uint32_t)&CLIENT_PRIVATE_DEVICE_CERT;
	for(i=0; i<len; i++) {
		if(HAL_OK != HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr+i, pdata[i])) {
			HAL_FLASH_Lock();
			return false;
		}
	}
	HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr+i, 0);
	HAL_FLASH_Lock();
	return true;
}

bool setMqttDestEndpoint(uint8_t *pdata, uint16_t len) {
	uint16_t i=0;
	HAL_FLASH_Unlock();
	uint32_t addr = (uint32_t)&mqttDestEndpoint;
	for(i=0; i<len; i++) {
		if(HAL_OK != HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr+i, pdata[i])) {
			HAL_FLASH_Lock();
			return false;
		}
	}
	HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr+i, 0);
	HAL_FLASH_Lock();
	return true;
}

bool setTopicPath(uint8_t *pdata, uint16_t len) {
	uint16_t i=0;
	HAL_FLASH_Unlock();
	uint32_t addr = (uint32_t)&mqttTopicPath;
	for(i=0; i<len; i++) {
		if(HAL_OK != HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr + i, pdata[i])) {
			HAL_FLASH_Lock();
			return false;
		}
	}
	HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr+i, 0);
	HAL_FLASH_Lock();
	return true;
}

bool setDeviceName(uint8_t *pdata, uint16_t len) {
	uint16_t i=0;
	HAL_FLASH_Unlock();
	uint32_t addr = (uint32_t)&mqttDeviceName;
	for(i=0; i<len; i++) {
		if(HAL_OK != HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr + i, pdata[i])) {
			HAL_FLASH_Lock();
			return false;
		}
	}
	HAL_FLASH_Program(TYPEPROGRAM_BYTE, addr+i, 0);
	HAL_FLASH_Lock();
	return true;
}

const char * getKeyCLIENT_CERTIFICATE_PEM() {
	return 	getKeyCLIENT_CERTIFICATE_PEM_IsExist() ? (char*)CLIENT_ROOT_CA : not_found_caption;
}

const char * getKeyCLIENT_PRIVATE_KEY_PEM() {
	return 	getKeyCLIENT_CERTIFICATE_PEM_IsExist() ? (char*)CLIENT_PRIVATE_KEY : not_found_caption;
}

const char * getKeyCLIENT_PRIVATE_DEVICE_CERT() {
	return 	getKeyCLIENT_CERTIFICATE_PEM_IsExist() ? (char*)CLIENT_PRIVATE_DEVICE_CERT : not_found_caption;
}

const char * getMqttDestEndpoint() {
	return 	getMqttDestEndpoint_isExist() ? (char*)mqttDestEndpoint : not_found_caption;
}

const char * getTopicPath() {
	return 	getTopicPath_isExist() ? (char*)mqttTopicPath : not_found_caption;
}

const char* getDeviceName() {
	return 	getDeviceName_isExist() ? (char*)mqttDeviceName : not_found_caption;
}

//--
//-- max length buff
//--
uint16_t getRootCaCertMaxSize() {
	return sizeof(CLIENT_ROOT_CA);
}

uint16_t getPrivateKeyMaxSize() {
	return sizeof(CLIENT_PRIVATE_KEY);
}

uint16_t getPrivateDeviceCertMaxSize() {
	return sizeof(CLIENT_PRIVATE_DEVICE_CERT);
}

uint16_t getMqttEndpointMaxSize() {
	return sizeof(mqttDestEndpoint);
}

uint16_t getTopicPathMaxSize() {
	return sizeof(mqttTopicPath);
}

uint16_t getDeviceNameMaxSize() {
	return sizeof(mqttDeviceName);
}

//---
//-- local functions
//--
bool flushSettingsFullSector() {
	bool res = false;
	uint32_t FirstSector = 0, NbOfSectors = 0, Address = 0;
	uint32_t SectorError = 0;
	__IO uint32_t data32 = 0 , MemoryProgramStatus = 0;

	/* Unlock the Flash to enable the flash control register access *************/
	HAL_FLASH_Unlock();

	// Erase the user Flash area
	// (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	/* Get the 1st sector to erase */
	FirstSector = GetSector(FLASH_USER_START_ADDR);
	/* Get the number of sector to erase from 1st sector*/
	NbOfSectors = GetSector(FLASH_USER_END_ADDR) - FirstSector + 1;

	/* Fill EraseInit structure*/
	FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase = TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange = VOLTAGE_RANGE_3;
	EraseInitStruct.Sector = FirstSector;
	EraseInitStruct.NbSectors = NbOfSectors;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
		// Error occurred while sector erase.
		// User can add here some code to deal with this error.
		// SectorError will contain the faulty sector and then to know the code error on this sector,
		// user can call function 'HAL_FLASH_GetError()' */
		// FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError();
		res = false;
	}

	// Program the user Flash area word by word
	// (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	Address = FLASH_USER_START_ADDR;

	while (Address < FLASH_USER_END_ADDR){
		if (HAL_FLASH_Program(TYPEPROGRAM_WORD, Address, DATA_32) == HAL_OK) {
			Address = Address + 4;
		} else { // Error occurred while writing data in Flash memory
			// User can add here some code to deal with this error */
			// FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError();
			res = false;
		}
	}

	/* Lock the Flash to disable the flash control register access (recommended
	 * to protect the FLASH memory against possible unwanted operation) */
	HAL_FLASH_Lock();

	/* Check if the programmed data is OK
    MemoryProgramStatus = 0: data programmed correctly
    MemoryProgramStatus != 0: number of words not programmed correctly ******/
	Address = FLASH_USER_START_ADDR;
	MemoryProgramStatus = 0x0;

	while (Address < FLASH_USER_END_ADDR) {
		data32 = *(__IO uint32_t*)Address;

		if (data32 != DATA_32) {
			MemoryProgramStatus++;
		}
		Address = Address + 4;
	}

	/*Check if there is an issue to program data*/
	if (MemoryProgramStatus == 0) { /* No error detected */
		res = true;
	}

	return res;
}

/**
 * @brief  Gets the sector of a given address
 * @param  None
 * @retval The sector of a given address
 */
static uint32_t GetSector(uint32_t Address) {
	uint32_t sector = 0;

	if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
	{
		sector = FLASH_SECTOR_0;
	}
	else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
	{
		sector = FLASH_SECTOR_1;
	}
	else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
	{
		sector = FLASH_SECTOR_2;
	}
	else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
	{
		sector = FLASH_SECTOR_3;
	}
	else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
	{
		sector = FLASH_SECTOR_4;
	}
	else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
	{
		sector = FLASH_SECTOR_5;
	}
	else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
	{
		sector = FLASH_SECTOR_6;
	}
	else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
	{
		sector = FLASH_SECTOR_7;
	}
	else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
	{
		sector = FLASH_SECTOR_8;
	}
	else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
	{
		sector = FLASH_SECTOR_9;
	}
	else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
	{
		sector = FLASH_SECTOR_10;
	}
	else /* (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11) */
	{
		sector = FLASH_SECTOR_11;
	}

	return sector;
}

