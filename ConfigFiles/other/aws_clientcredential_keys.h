#ifndef AWS_CLIENT_CREDENTIAL_KEYS_H
#define AWS_CLIENT_CREDENTIAL_KEYS_H

#include <stdint.h>

/*
 * PEM-encoded client certificate
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */

#define keyCLIENT_CERTIFICATE_PEM \
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
"-----END CERTIFICATE-----\r\n" \

/*
 * PEM-encoded issuer certificate for AWS IoT Just In Time Registration (JITR).
 * This is required if you're using JITR, since the issuer (Certificate
 * Authority) of the client certificate is used by the server for routing the
 * device's initial request. (The device client certificate must always be
 * sent as well.) For more information about JITR, see:
 *  https://docs.aws.amazon.com/iot/latest/developerguide/jit-provisioning.html,
 *  https://aws.amazon.com/blogs/iot/just-in-time-registration-of-device-certificates-on-aws-iot/.
 *
 * If you're not using JITR, set below to NULL.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
#define keyJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM    NULL

/*
 * PEM-encoded client private key.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN RSA PRIVATE KEY-----\n"\
 * "...base64 data...\n"\
 * "-----END RSA PRIVATE KEY-----\n"
 */
#define keyCLIENT_PRIVATE_KEY_PEM \
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
"-----END RSA PRIVATE KEY-----\r\n" \

#define keyCLIENT_PRIVATE_DEVICE_CERT_PEM \
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
"-----END CERTIFICATE-----\r\n" \

/* The constants above are set to const char * pointers defined in aws_dev_mode_key_provisioning.c,
 * and externed here for use in C files.  NOTE!  THIS IS DONE FOR CONVENIENCE
 * DURING AN EVALUATION PHASE AND IS NOT GOOD PRACTICE FOR PRODUCTION SYSTEMS
 * WHICH MUST STORE KEYS SECURELY. */
//extern const char clientcredentialCLIENT_CERTIFICATE_PEM[];
//extern const char * clientcredentialJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM;
//extern const char clientcredentialCLIENT_PRIVATE_KEY_PEM[];
//extern const uint32_t clientcredentialCLIENT_CERTIFICATE_LENGTH;
//extern const uint32_t clientcredentialCLIENT_PRIVATE_KEY_LENGTH;

#endif /* AWS_CLIENT_CREDENTIAL_KEYS_H */
