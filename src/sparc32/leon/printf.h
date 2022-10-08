/* leonbare functions do not support %llx etc., so always use pg_ versions */
#define sprintf pg_sprintf
#define fprintf pg_fprintf
#define printf pg_printf
#define vsnprintf pg_vsnprintf
#define snprintf pg_snprintf

