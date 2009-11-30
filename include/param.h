
#define	KWMD_MAX_STRING 1024

/*******
 * Feature-related options.
 * XXX: WIP
 */
typedef enum _featurelist {
	FEAT_REPLACE = 0,
	FEAT_SYNC,
	FEAT_NUM
} featurelist;

typedef enum _featuretype {
	FTYPE_BOOL = 0,
	FTYPE_UINT,
	FTYPE_INT,
	FTYPE_STRING,
	FTYPE_KEY,
	FTYPE_NUM
} featuretype;

/* Container for data types available for features, passed to various
 * helper-functions.
 */
typedef union _t_data {
	void *v;
	char *str;
	int i;
	unsigned int u;
	int b;
} t_data;

/* Test for various generic integer feature-functions.  */
#define FTYPE_IS_INT(s) (s == FTYPE_INT || s == FTYPE_UINT || s == FTYPE_BOOL)


extern int ftype_check_int(
	const featuretype type,
	const int min,
	const int max,
	const t_data data);

extern int ftype_check_string(
	const featuretype type,
	const int min,
	const int max,
	const t_data data);

extern int ftype_check_key(
	const featuretype type,
	const int min,
	const int max,
	const t_data data);

typedef int (*verify_function)(
	const featuretype type,
	const int min,
	const int max,
	const t_data data);

/* Different feature-types. The min/max is passed to the verification
 * function to allow re-use of verification functions in case of int, uint
 * and bool.
 */
typedef struct _t_ftype {
	const char *desc;
	int min;
	int max;
	verify_function verify;
} t_ftype;
typedef enum _t_feature_enum {
	F_REPLACE = 0,
	F_SYNC,
	F_VERBOSITY,
	F_NUM
} t_feature_enum;

typedef struct _t_feature {
	const char *name;
	const char *description;
	featuretype type;
	t_data d; // data/value. Frequently used shorthand.
} t_feature;

extern t_feature feature[];
extern t_ftype ftype[];

int verify_all_features(void);
int verify_feature(const t_feature *feature);

