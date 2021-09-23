#if defined WINDOWS || defined _WIN32 || defined HAVE_WINDOWS_H
#define  WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "portalext.h"
#include "pbo_decimal.h"
#include "st.h"
#include "ruby.h"

/* Prototype */
static pin_flist_t * portal_hash_to_flist(VALUE hash);
static VALUE portal_to_flist_array_i(VALUE pair, VALUE data);
static int portal_to_flist_i(VALUE key, VALUE val, ConvertData *cd);

/* Class visible values */
VALUE mPortal, cContext, ePortalError;

/* Ids and constants */
static ID id_call;
static ID id_inspect;
static ID id_to_f;
static ID id_to_i;
static ID id_to_s;
static ID id_to_sym;
static ID id_xop;

/*
 * Free the
 * Called by ruby as part of the initialization.
 */
void
pdata_free(pd)
  PortalData *pd;
{
  TRACE("Freeing");
  PDataClearErr(pd);
  if (pd->ctxp != NULL){
    PCM_CONTEXT_CLOSE(pd->ctxp,0,&pd->ebuf);
  }
  PIN_FLIST_DESTROY_EX(&(pd->in_flistp), NULL);
  PIN_FLIST_DESTROY_EX(&(pd->out_flistp), NULL);
}

void
cdata_free(cd)
  ConvertData *cd;
{
  PIN_FLIST_DESTROY_EX(&(cd->flistp), NULL);
}

/*
 *  call-seq:
 *     Connection.new -> a naked object
 *  Allocate the C data structure that follows this instance around.
 *  Called by ruby as part of the initialization.
 *  We return a brand new Ruby object.
 */
static VALUE portal_alloc _((VALUE));
static VALUE
portal_alloc(klass)
    VALUE klass;
{
  VALUE obj;
  PortalData *pd = NULL;
  PDataWrap(klass,obj,pd);
  TRACE_INSPECT("Allocated",obj);
  return obj;
}

/*
 *  call-seq:
 *     Connection.inspectx -> Dumps the associated C structs
 */
static VALUE
portal_inspect(self)
  VALUE self;
{
  char *cname = rb_obj_classname(self);
  size_t len;
  VALUE str;
  unsigned long id = rb_obj_id(self);
  char *format = "#<%s:0X%lx owner_id=0x%lx>";

  PortalData *pd;
  PDataGet(self,pd);
  TRACE("Inspecting");

  len = strlen(cname)+6+16;
  // len = strlen(format);
  str = rb_str_new(0, len); /* 6:tags 16:addr */
  snprintf(RSTRING_PTR(str), len+1, "#<%s:0X%lx>", cname, self);
  //RSTRING_LEN(str) = strlen(RSTRING_PTR(str));
  if (OBJ_TAINTED(self)) OBJ_TAINT(str);

  return str;

}

/*
 * Cast something into a string.
 */
#if defined WINDOWS || defined _WIN32 || defined HAVE_WINDOWS_H
static __inline VALUE
#else
static VALUE
#endif
portal_to_s(obj)
  VALUE obj;
{
  return rb_obj_as_string(obj);
  return rb_funcall(obj, id_to_s, 0, 0);
}

/*
 * Cast a String or Symbol into a char *.
 */
#if defined WINDOWS || defined _WIN32 || defined HAVE_WINDOWS_H
static __inline char*
#else
static char*
#endif
portal_to_char(str)
  VALUE str;
{
  VALUE tmp;
  if (TYPE(str) == T_STRING){
    return StringValuePtr(str);
  }
  tmp = rb_funcall(str, id_to_s, 0, 0);
  return StringValuePtr(tmp);
}

/*
 * The Portal field number for a string.
 */
static VALUE
portal_field_num(obj,str)
VALUE obj, str;
{
  pin_fld_num_t fld_num = 0;
  VALUE value;

  if (TYPE(str) == T_STRING){
    fld_num = PIN_FIELD_OF_NAME(StringValuePtr(str));
  } else {
    value = portal_to_s(str);
    fld_num = PIN_FIELD_OF_NAME(portal_to_char(value));
  }

  if (!fld_num){
    rb_raise(ePortalError, "Cannot find field number for %s", StringValuePtr(str));
  }
  return INT2FIX(PIN_GET_NUM_FROM_FLD(fld_num));
}

/*
 * Returns the name for a given field number.
 */
static VALUE
portal_field_name(self,num)
  VALUE self, num;
{
  VALUE tmp = Qundef;
  VALUE name = Qundef;
  const char *fld_name = NULL;

  tmp = rb_funcall(num, id_to_i, 0, 0);

  fld_name = pin_name_of_field(FIX2INT(tmp));
  name = rb_str_new2(fld_name);
  return name;
}

/*
 * The Portal field type.
 */
static VALUE
portal_field_type(obj, value)
VALUE obj, value;
{
  int i = 0;
  i = PIN_FIELD_OF_NAME(portal_to_char(value));
  i = PIN_GET_TYPE_FROM_FLD(i);
  if (!i){
    rb_raise(ePortalError, "Cannot get type for %s",portal_to_char(value));
  }
  return INT2FIX(i);
}

/*
 *  TODO: Make robust enough to handle /account 1.
 *  Perhaps add: sscanf(portal_to_char(val),"0.0.0.%lld %s %lld",&poid_db,buf,&poid_id0);
 */
static poid_t *
portal_poid_from_string(string,ebufp)
  char * string;
  pin_errbuf_t    *ebufp;
{
  // int64  poid_id0,poid_db;
  // static char buf[64];
  poid_t *pdp;
  pdp = PIN_POID_FROM_STR(string,NULL,ebufp);
  return pdp;
}

/*
 *  call-seq:
 *    portal_to_flist(hash) => Portal FList
 *  An error is kept in a local ebuf untill the end.
 */
//static VALUE
static int
portal_to_flist_i(VALUE key,VALUE val,ConvertData *cd)
{
  pin_buf_t *pin_bufp = NULL;
  pin_errbuf_t ebuf;
  pin_fld_num_t fld_num = 0;
  pin_flist_t *ary_flistp, *save_flistp;
  int fld_type = 0;
  int int_value = 0;
  double big_value = 0.0;
  char *fld_name = NULL, *tmp_str = NULL;
  pin_decimal_t *pbo = NULL;
  poid_t *pdp = NULL;

  if (key == Qundef) {
    return ST_CONTINUE;
  }
  PIN_ERR_CLEAR_ERR(&ebuf);

  fld_name = portal_to_char(key);
  fld_num = PIN_FIELD_OF_NAME(fld_name);
  if (!fld_num) {
    rb_raise(ePortalError, "Cannot find field number for %s", fld_name);
  }

  fld_type = PIN_GET_TYPE_FROM_FLD(fld_num);
  if (!fld_type){
    rb_raise(ePortalError, "Cannot get type for %s", fld_name);
  }

#ifdef _DEBUG
  if (TYPE(val) != T_HASH)
    tmp_str = portal_to_char(val);
  else
    tmp_str = "Hash";
  if (rb_gv_get("$DEBUG"))
  fprintf(stderr,"  hash2flist name=%s num=%ld type=%ld val=%s\n", fld_name, fld_num, fld_type, tmp_str);
#endif

  switch(fld_type){
    case PIN_FLDT_STR:
      PIN_FLIST_FLD_SET(cd->flistp, fld_num, (void *) portal_to_char(val), &ebuf);
      break;

    case PIN_FLDT_INT:
    case PIN_FLDT_UINT:
    case PIN_FLDT_ENUM:
    case PIN_FLDT_TSTAMP:
      int_value = TO_INT(val);
      PIN_FLIST_FLD_SET(cd->flistp, fld_num, (void *)&int_value, &ebuf);
      break;

    case PIN_FLDT_NUM:
    case PIN_FLDT_DECIMAL:
      big_value = TO_BIG(val);
      pbo = pbo_decimal_from_double(big_value,&ebuf);
      PIN_FLIST_FLD_SET(cd->flistp, fld_num, (void *)pbo, &ebuf);

      if (! pbo_decimal_is_null(pbo,&ebuf)) {
        pbo_decimal_destroy(&pbo);
      }
      break;

    case PIN_FLDT_POID:
      /* Parse 0.0.0.1 /account 1234 1 */
      pdp = portal_poid_from_string(portal_to_char(val), &ebuf);
      PIN_FLIST_FLD_PUT(cd->flistp, fld_num, (void *)pdp, &ebuf);
      break;

    case PIN_FLDT_ARRAY:
      Check_Type(val,T_HASH);
      cd->fld_num = fld_num;
      save_flistp = cd->flistp;
      rb_iterate(rb_each, val, portal_to_flist_array_i,(VALUE)cd);
      (cd)->flistp = save_flistp; /* Restore the original flist. */
      (cd)->fld_num = fld_num;
      ebuf = *cd->ebufp;
      break;

    case PIN_FLDT_SUBSTRUCT:
      /* Swap out the flist in the structure. */
      Check_Type(val,T_HASH);
      save_flistp = cd->flistp;
      ary_flistp = PIN_FLIST_SUBSTR_ADD(cd->flistp, fld_num, &ebuf);
      (cd)->flistp = ary_flistp;
      rb_hash_foreach(val, portal_to_flist_i,cd);
      (cd)->flistp = save_flistp;
      ebuf = *cd->ebufp;
      break;

    case PIN_FLDT_BUF:
fprintf(stderr,"XXX portal_to_flist_i\n");    
      if ((val == Qundef) ||(val == Qnil)){
fprintf(stderr,"XXX Qundef or Qnil\n");    
        PIN_FLIST_FLD_SET(cd->flistp, fld_num, NULL, &ebuf);
      } else {
        pin_bufp = (pin_buf_t *)calloc(1, sizeof(pin_buf_t));
        pin_bufp->data = RSTRING_PTR(val);
        pin_bufp->size= RSTRING_LEN(val);
        PIN_FLIST_FLD_SET(cd->flistp, fld_num, pin_bufp, &ebuf);
        free(pin_bufp);
      }
      break;
    case PIN_FLDT_OBJ:
    case PIN_FLDT_BINSTR:
    case PIN_FLDT_ERR:
    default:
      rb_raise(ePortalError, "Type %ld not supported for %s", fld_type, fld_name);
      break;
  } /* End switch */

  if (PIN_ERR_IS_ERR(&ebuf)) {
    rb_raise(ePortalError, "Cannot convert %s", fld_name);
    PIN_ERR_LOG_EBUF(PIN_ERR_LEVEL_ERROR, "Hash to flist conversion error", &ebuf);
  }

  /* Set the error - but only once */

  return ST_CONTINUE;
}

/*
 *  Interates through a nested Hash to create a nested flist.
 *  The hash keys must respond to to_i, as that value becomes the REC ID in the array.
 *
 *  === Example:
 *
 *    hash = {
 *      "PIN_FLD_NAME" => "Foo",
 *      :PIN_FLD_ARGS  => {
 *        1            => { :PIN_FLD_NAME => "Name 0" },
 *        2            => {
 *          :PIN_FLD_NAME           => "Name 1",
 *          :PIN_FLD_CREATED_T      => 1,
 *        },
 *        3            => {
 *          :PIN_FLD_NAME           => "Value 2",
 *          :PIN_FLD_CREATED_T      => 2,
 *        },
 *      },
 *    }
 *  TODO: What about PIN_FLIST_ANY?
 */
static VALUE
portal_to_flist_array_i(pair,data)
  VALUE pair, data;
{
  ConvertData *cd = (ConvertData *)data;
  VALUE key, val;
  pin_flist_t *ary_flistp, *save_flistp;
  int rec_id = 0;
  VALUE tmp_obj;
  pin_fld_num_t fld_num;

  //CDataGet(data,cd);
  Check_Type(pair,T_ARRAY);
  key = rb_ary_entry(pair, 0);
  val = rb_ary_entry(pair, 1);
  fld_num = cd->fld_num;

  if (TYPE(key) == T_FIXNUM) {
    rec_id = FIX2INT(key);
  } else {
    tmp_obj = rb_funcall(key, id_to_i, 0, 0);
    rec_id = FIX2INT(tmp_obj);
  }

  save_flistp = cd->flistp;
  ary_flistp = PIN_FLIST_ELEM_ADD(cd->flistp, fld_num, rec_id, cd->ebufp);

  cd->flistp = ary_flistp;
  rb_hash_foreach(val, portal_to_flist_i,cd);
  cd->flistp = save_flistp;
  return ST_CONTINUE;
}

/*
 * Convert a Ruby hash into a Portal flist
 */
static pin_flist_t *
portal_hash_to_flist(hash)
  VALUE hash;
{
  pin_errbuf_t  ebuf;
  ConvertData cdata;

  cdata.flistp = NULL;
  cdata.ebufp = &ebuf;

  // TODO: rb_thread_critical = Qtrue;
  Check_Type(hash,T_HASH);
  PIN_ERR_CLEAR_ERR(&ebuf);
  cdata.flistp = PIN_FLIST_CREATE(&ebuf);

  /*  Iterate through the hash converting each key/val pair into
   *  their Portal equivilent.
   *  Not using rb_iterate(rb_each, hash, portal_to_flist_i, hash);
   *  because the receiver gets an array of [key,val] which you have to noodle.
   *
   *  Using rb_hash_foreach(hash, inter_i, struct);
   *  Calls inter_i for each key/val pair in the hash.
   *  iter_i has prototype inter_i(key,val,struct).
   *    key and val are obvious. struct is some data structure
   *    that you use to maintain state between calls to the inter.
   */
  rb_hash_foreach(hash, portal_to_flist_i,&cdata);

  PIN_ERR_CLEAR_ERR(&ebuf);
  return cdata.flistp;
}

/*
 * Converts Portal Poid to Ruby String. Returns a Ruby String.
 */
static VALUE
portal_poid_to_val(pdp)
  poid_t *pdp;
{
  pin_errbuf_t ebuf;
  int buf_size = PCM_MAX_POID_TYPE + 48;
  char buf[PCM_MAX_POID_TYPE + 48];
  char *bufp = NULL;
  memset(buf,0,sizeof(buf));
  bufp = buf;
  PIN_ERR_CLEAR_ERR(&ebuf);
  PIN_POID_TO_STR((poid_t *)pdp, &bufp, &buf_size, &ebuf);
  return rb_str_new2(bufp);
}

/*
 * Convert an Flist into a hash.
 */
static VALUE
portal_flist_to_hash(flistp)
  pin_flist_t *flistp;
{
  pin_buf_t *pin_bufp = NULL;
  pin_fld_num_t fld_num = 0;
  int buf_size = PCM_MAX_POID_TYPE + 48;
  char buf[PCM_MAX_POID_TYPE + 48];
  char *bufp = NULL;
  const char *fld_name = NULL;
  void *field_val = NULL;
  int rec_id = 0;
  int fld_type = 0;
  pin_cookie_t cookie = NULL;
  pin_cookie_t ary_cookie = NULL;
  double dbl = 0.00;
	VALUE subhash = Qundef;
  pin_errbuf_t ebuf;
  ConvertData cdata;
  VALUE hash = Qundef, key = Qundef, val = Qundef;

  cdata.flistp = NULL;
  cdata.ebufp = &ebuf;

  PIN_ERR_CLEAR_ERR(&ebuf);
  cdata.flistp = PIN_FLIST_CREATE(&ebuf);

  memset(buf,0,sizeof(buf));
  bufp = buf;
  hash = rb_hash_new();

  while ((field_val = pin_flist_any_get_next(flistp, &fld_num, &rec_id, NULL, &cookie, &ebuf)) != NULL) {
    fld_name = pin_name_of_field(fld_num);
    key = rb_str_new2(fld_name);
    fld_type = PIN_GET_TYPE_FROM_FLD(fld_num);
    switch(fld_type) {
    case PIN_FLDT_STR:
      val = rb_str_new2(field_val);
      break;

    case PIN_FLDT_INT:
    case PIN_FLDT_UINT:
    case PIN_FLDT_ENUM:
    case PIN_FLDT_TSTAMP:
      val = INT2NUM(*(int*)field_val);
      break;

    case PIN_FLDT_NUM:
    case PIN_FLDT_DECIMAL:
      dbl = pbo_decimal_to_double((pin_decimal_t *)field_val, &ebuf);
      val = rb_float_new(dbl);
      break;

    case PIN_FLDT_POID:
      buf_size = sizeof(buf); // b/c this changes it and memset(buf,0,sizeof(buf));
      PIN_POID_TO_STR((poid_t *)field_val, &bufp, &buf_size, &ebuf);
      val = rb_str_new2(bufp);
      break;

    case PIN_FLDT_ARRAY:
			subhash = rb_hash_aref(hash, key);
			if (TYPE(subhash) != T_HASH){
				subhash = rb_hash_new();
				rb_hash_aset(hash,key,subhash);
			}

			val = portal_flist_to_hash((pin_flist_t *) field_val);
			rb_hash_aset(subhash,INT2FIX(rec_id),val);

      goto SKIP_SET;
      break;

    case PIN_FLDT_SUBSTRUCT:
       /* hv_store(h,fld_name,fld_name_len,
         flist_to_hash((pin_flist_t *) field_val), 0);
         */
      rb_raise(ePortalError, "struct=>hash not implemented for key=%s", portal_to_char(key));
      break;
case PIN_FLDT_BUF:
  pin_bufp = field_val;
if (pin_bufp->size == 0){
fprintf(stderr,"BUF setting nil value\n");
  val = Qundef;
  VALUE myNil = Qnil;
  rb_hash_aset(hash, key, myNil);
  // rb_hash_aset(hash, key, rb_str_new2(""));
} else {
fprintf(stderr,"BUF setting non-nil value\n");
  val = rb_str_new(pin_bufp->data, pin_bufp->size);
}
		// buft.flag       = 0;
		// buft.size       = 0;
		// buft.offset     = 0;
		// buft.data       = 0;
		// buft.xbuf_file  = NULL;

break;
    case PIN_FLDT_BINSTR:
    case PIN_FLDT_ERR:
    case PIN_FLDT_OBJ:
    default:
      fprintf(stderr,"  flist2hash name=%s num=%ld type=%ld\n", portal_to_char(key), fld_num, fld_type);
      break;
    }

    if (PIN_ERR_IS_ERR(&ebuf)){
      PIN_ERR_LOG_EBUF(PIN_ERR_LEVEL_ERROR, "Flist to hash conversion error", &ebuf);
      rb_raise(ePortalError, "Error converting flist to hash key=%s type=%ld", portal_to_char(key),fld_type);
    }

    if (val != Qundef){
      rb_hash_aset(hash, key, val);
#ifdef _DEBUG
    if (rb_gv_get("$DEBUG"))
      fprintf(stderr,"  flist2hash name=%s num=%ld type=%ld val=%s\n", portal_to_char(key), fld_num, fld_type, portal_to_char(val));
#endif
      val = Qundef;
    }

SKIP_SET:
;
  }

  PIN_ERR_CLEAR_ERR(&ebuf);

  Check_Type(hash,T_HASH);
  return hash;
}

/*
 * Test function for the from flist to hash conversion.
 */
#ifdef _DEBUG
static VALUE
portal_test_flist_to_hash(self,hash)
  VALUE self, hash;
{
  VALUE result;
  pin_flist_t *flistp;
  pin_errbuf_t ebuf;
  //char *flist_str = NULL;
  //int flist_len = NULL;

  PIN_ERR_CLEAR_ERR(&ebuf);
  flistp = portal_hash_to_flist(hash);
  PIN_ERR_LOG_FLIST(PIN_ERR_LEVEL_DEBUG, "portal_test_flist_to_hash flist", flistp);
  result = portal_flist_to_hash(flistp);
  PIN_ERR_LOG_EBUF(PIN_ERR_LEVEL_DEBUG, "portal_test_flist_to_hash ebufp", &ebuf);
  //flist_str = NULL;
  //PIN_FLIST_TO_STR(flistp, &flist_str, &flist_len, &ebuf);
  PIN_FLIST_DESTROY_EX(&flistp,NULL);

  return result;
}
#endif

/*
 * Test function for the from flist to hash conversion.
 */
static VALUE
portal_hash_to_flist_string(self,hash)
  VALUE self, hash;
{
  VALUE result;
  pin_flist_t *flistp;
  pin_errbuf_t ebuf;
#if defined WINDOWS || defined _WIN32 || defined HAVE_WINDOWS_H
  /* For some reason, windows does like other people giving us heap. */
  int flist_len = 1000000;
  char *flist_str = malloc(flist_len);
#else
  int flist_len = NULL;
  char *flist_str = NULL;
#endif

  PIN_ERR_CLEAR_ERR(&ebuf);
  flistp = portal_hash_to_flist(hash);
  PIN_ERR_LOG_FLIST(PIN_ERR_LEVEL_DEBUG, "portal_hash_to_flist_string return", flistp);
  PIN_FLIST_TO_STR(flistp, &flist_str, &flist_len, &ebuf);
  PIN_FLIST_DESTROY_EX(&flistp,NULL);
  result = rb_str_new2(flist_str);
  free(flist_str);
  return result;
}

/*
 * Document-method: set_program_name
 *
 *   call-seq:
 *     Portal.set_program_name(name) -> true
 *
 *   Set the program name for the Portal log file.
 */
static VALUE
portal_set_program_name(self,name)
  VALUE self, name;
{
  PIN_ERR_SET_PROGRAM(portal_to_char(name));
  return Qtrue;
}

/*
 * Document-method: set_log_level
 *
 *   call-seq:
 *     Portal.set_log_level(level) -> true
 *   example:
 *     Portal.set_log_level(:warn)
 *     Portal.set_log_level(2)
 *
 *  Set the error level for the log file using symbols, strings, or ints.
 */
static VALUE
portal_set_log_level(self,level)
  VALUE self, level;
{
  int lev = 0;
  char *cp = NULL;

  if (TYPE(level) == T_STRING || TYPE(level) == T_SYMBOL) {
    cp = portal_to_char(level);
    if (!strcmp(cp,"error")){
      lev = PIN_ERR_LEVEL_ERROR;
    } else if (!strcmp(cp,"warn")) {
      lev = PIN_ERR_LEVEL_WARNING;
    } else if (!strcmp(cp,"debug")) {
      lev = PIN_ERR_LEVEL_DEBUG;
    } else {
      rb_raise(ePortalError, "Error level %s not supported", cp);
    }
  } else {
    lev = NUM2INT(level);
    if (lev > PIN_ERR_LEVEL_DEBUG){
      rb_raise(ePortalError, "Error level %s not supported", cp);
    }
  }
  PIN_ERR_SET_LEVEL(lev);
  return Qtrue;
}

#define PORTAL_CHECK_ERROR(level,msg,flistp,ebufp) \
  if (PIN_ERR_IS_ERR(ebufp)){ \
    PIN_ERR_LOG_FLIST(level,msg,flistp); \
    rb_raise(ePortalError, "Ebuf error: %s", msg); \
  }

/*
 * Get value in hash by key. Try a symbol and string.
 */
VALUE
portal_get_hash_value(hsh, key)
    VALUE hsh, key;
{
  VALUE val = rb_hash_aref(hsh,key);
  if (NIL_P(val)) {
  }
  return val;
}

/*
 *  Document-method: xop
 *
 *  == Summary
 *    Perform a Portal opcode by converting a hash into a flist.
 *
 *   Options:
 *     The options hash takes the following keys:
 *       :return | :flist_string | Converts the results flist into an flist_to_str like testnap
 *       :flags
 *
 *   call-seq:
 *     ph.xop() -> Hash -or- Flist str
 *   example:
 *     ph.xop(:PCM_OP_READ_OBJ,{:PIN_FLD_POID=>"0.0.0.1 /account 1"},:return => :flist_string)
 */
static VALUE
portal_xop(argc, argv, self)
    int argc;
    VALUE *argv;
    VALUE self;
{
  static ID id_flist_string = NULL;
  static ID id_return = NULL;

  VALUE response, tmp;
  VALUE opcode, request, args;

  pin_errbuf_t ebuf;
  pin_flist_t *in_flistp = NULL;
  pin_flist_t *out_flistp = NULL;
  pin_opcode_t op;
  PortalData *pd;
#if defined WINDOWS || defined _WIN32 || defined HAVE_WINDOWS_H
  /* For some reason, windows does like other people giving us heap. */
  int flist_len = 1000000;
  char *flist_str = malloc(flist_len);
#else
  int flist_len = NULL;
  char *flist_str = NULL;
#endif
  int return_string = 0;

  rb_scan_args(argc, argv, "21", &opcode, &request, &args);

  if (id_flist_string == NULL){
    id_flist_string = rb_intern("flist_string");
    id_return = rb_intern("return");
  }

  if (TYPE(args) == T_HASH){
    TRACE_INSPECT("Hash of args",args);
    tmp = rb_hash_aref(args, ID2SYM(id_return));
    if (TYPE(tmp) == T_SYMBOL && tmp == ID2SYM(id_flist_string)){
      return_string = 1;
    }
  }

  op = pcm_opname_to_opcode(portal_to_char(opcode));
  TRACE_FUNC("xop","Opcode",pcm_opcode_to_opname(op));
  in_flistp = portal_hash_to_flist(request);
  PDataGet(self,pd);
  PIN_ERR_CLEAR_ERR(&ebuf);
  PCM_OP(pd->ctxp, op, 0, in_flistp, &out_flistp, &ebuf);
  pd->ops++;
  PORTAL_CHECK_ERROR(PIN_ERR_LEVEL_ERROR,"Bad operation",out_flistp,&ebuf);
  if(PIN_ERR_IS_ERR(&ebuf)){
    pd->errors++;
    TRACE("Ebuf has error");
    PIN_ERR_LOG_EBUF(PIN_ERR_LEVEL_ERROR, "xop error", &ebuf);
    PIN_ERR_LOG_FLIST(PIN_ERR_LEVEL_ERROR, "input flist", in_flistp);
    PIN_ERR_LOG_FLIST(PIN_ERR_LEVEL_ERROR, "return flist", out_flistp);
  } else if (rb_gv_get("$DEBUG")) {
    PIN_ERR_LOG_MSG(PIN_ERR_LEVEL_DEBUG,  "xop input/output");
    PIN_ERR_LOG_FLIST(PIN_ERR_LEVEL_DEBUG, "input flist", in_flistp);
    PIN_ERR_LOG_FLIST(PIN_ERR_LEVEL_DEBUG, "return flist", out_flistp);
  } else {
    TRACE("xop success");
  }

  if (return_string){
    PIN_FLIST_TO_STR(out_flistp, &flist_str, &flist_len, &ebuf);
    //rb_str_new(flist_str,flist_len);
		response = rb_hash_new();
		rb_hash_aset(response,ID2SYM(id_flist_string),rb_str_new(flist_str,flist_len));
		rb_hash_aset(response,rb_str_new2(":response"),portal_flist_to_hash(out_flistp));
    free(flist_str);
  } else {
    response = portal_flist_to_hash(out_flistp);
  }
  TRACE_INSPECT("Response",response);
  PIN_FLIST_DESTROY_EX(&in_flistp, NULL);
  PIN_FLIST_DESTROY_EX(&out_flistp, NULL);
  TRACE("xop returning");
  return response;
}

static VALUE
portal_loopback(self)
  VALUE self;
{
  VALUE hash;
  TRACE("Running loopback");
  hash = rb_hash_new();
  rb_hash_aset(hash,rb_str_new2("PIN_FLD_POID"),rb_str_new2("0.0.0.1 /account 1 0"));
  return rb_funcall(self, id_xop, 3, rb_str_new2("PCM_OP_TEST_LOOPBACK"), hash, Qnil);
}

/*
 *  Document-method: session
 *
 *  == Summary
 *    Returns the session associated to the current context.
 *    Returns nil if not connected.
 */
static VALUE
portal_get_session(self)
{
  PortalData *pd;
  TRACE("start");
  PDataGet(self,pd);
  if (pd->ctxp == NULL)
    return Qnil;
  return portal_poid_to_val(pcm_get_session(pd->ctxp));
}

/*
 *  Document-method: userid
 *
 *  === Summary
 *
 *    Returns the user poid associated to the current context.
 *    Returns nil if not connected.
 */
static VALUE
portal_get_userid(self)
{
  PortalData *pd;
  TRACE("start");
  PDataGet(self,pd);
  if (pd->ctxp == NULL)
    return Qnil;
  return portal_poid_to_val(pcm_get_userid(pd->ctxp));
}

/*
 *  Document-method: robj
 *
 *  === Summary
 *
 *    Read an object.
 *
 *  === Example
 *
 *    portal.robj :PIN_FLD_POID => "0.0.0.1 /account 1"
 *    portal.robj "0.0.0.1 /account 1"
 */
static VALUE
portal_robj(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
  VALUE read_obj = NULL;
  VALUE request, args, hash;
  rb_scan_args(argc, argv, "11", &request, &args);

  if (TYPE(request) == T_HASH) {
    hash = request;
  } else if (TYPE(request) == T_STRING) {
    hash = rb_hash_new();
    rb_hash_aset(hash,rb_str_new2("PIN_FLD_POID"),request);
  } else {
    rb_raise(ePortalError, "Expected Hash or String.");
  }

  if (read_obj == NULL){
    read_obj = rb_str_new2("PCM_OP_READ_OBJ");
  }

  return rb_funcall(obj, id_xop, 3, read_obj, hash, args);
}

/*
 *  Document-method: connect
 *
 *  === Summary
 *
 *    Connect to portal using the supplied details.
 *    If hash, use those details.
 *    If string, then that is the location for our pin.conf
 *    If nil, then we assume out cwd has the pin.conf.
 *
 *  === Example
 *
 *    ctxp.connect("/usr/local/portal/sys/test")
 *
 *  This method avoids the need for a pin.conf. Note that this is a cmmp login type, so the password is not needed.
 *	  request = {
 *	  	:PIN_FLD_POID => "0.0.0.1 /service/pcm_client 1",
 *		  :PIN_FLD_TYPE => 0,
 *  		:PIN_FLD_CM_PTR => {
 *	  		0 => { :PIN_FLD_CM_PTR => "ip portal01.prod.example.com 11960"}
 *		  	1 => { :PIN_FLD_CM_PTR => "ip portal02.prod.example.com 11960"}
 *		  }
 *		ctxp.connect(request)
 */
static VALUE
portal_connect(self,obj)
  VALUE self, obj;
{
  PortalData *pd;
  pcm_context_t *ctxp = NULL;
  int64 database;// = 0;
  static char * cwd = NULL;
  pin_flist_t *in_flistp = NULL;

  TRACE("Connecting");
  PDataGet(self,pd);
  PDataClearErr(pd);

  if (TYPE(obj) == T_NIL ) {
    PCM_CONNECT(&ctxp,&database,&pd->ebuf);
  } else if (TYPE(obj) == T_HASH ) {
    TRACE_INSPECT("Hash of args",obj);
		in_flistp = portal_hash_to_flist(obj);
		PCM_CONTEXT_OPEN(&ctxp,in_flistp,&pd->ebuf);
	  PIN_FLIST_DESTROY_EX(&in_flistp, NULL);
  } else if (TYPE(obj) == T_STRING ) {

    // Only do this once. getwd is not thread smart.
    if (cwd == NULL ) {
      cwd = ruby_getcwd();
      if (cwd == NULL) {
        rb_raise(ePortalError, "getcwd failed.");
      }
    }

    /* Move to that directory */
    if (chdir(portal_to_char(obj)) < 0) {
      rb_raise(ePortalError, "Cannot cd to %s",portal_to_char(obj));
    }

    PCM_CONNECT(&ctxp,&database,&pd->ebuf);

    /* Return to the original directory */
    if (chdir(cwd) < 0) {
      rb_raise(ePortalError, "Cannot cd to original dir %s",cwd);
    }

  } else {
    rb_raise(ePortalError, "Unable to connect with supplied args.");
  }

  if (PIN_ERR_IS_ERR(&pd->ebuf)) {
    TRACE("Bad Connect");
    pd->ctxp = NULL;
		if (pd->ebuf.pin_err == PIN_ERR_BAD_LOGIN_RESULT){
	    rb_raise(ePortalError, "Bad login result.");
		}
    rb_raise(ePortalError, "Cannot connect.");
  }
  TRACE("Connected");
  pd->ctxp = ctxp;
  return self;
}

/*
 *  Document-method: disconnect
 *
 *  === Summary
 *
 *    Disconnect from Portal. This is important to avoid mem leaks.
 *
 *  === Example
 *
 *    ph.disconnect()
 */
static VALUE
portal_disconnect(self)
  VALUE self;
{
  PortalData *pd;
  PDataGet(self,pd);
  TRACE("Disconnecting");
  PCM_CONTEXT_CLOSE(pd->ctxp,0,&pd->ebuf);
  pd->ctxp = NULL;
  TRACE("Disconnected");
	return self;
}

static VALUE
portal_initialize(VALUE self)
{
  if (rb_gv_get("$DEBUG")){
    PIN_ERR_SET_LEVEL(PIN_ERR_LEVEL_DEBUG);
    fprintf(stderr,"Portal logging set to debug\n");
  }

  return self;
}

/*
 *  Document-class: Portal::Connection
 *
 *  == Summary
 *
 *  A single connection to Portal that one can use to execute opcodes.
 *
 *  === Attributes
 *
 *  ctxp:: The context pointer for the current connection.
 *  program_name::
 *  loglevel::
 *
 */

/*
 *  Document-class: Portal::PortalError
 *
 *  === Summary
 *
 *  Raise when ebuf contains errors.
 */

/*
 *  Document-module: Portal
 *
 *  === Summary
 *
 *  === Example
 *  none.
 */
void
Init_portalext(void)
{
   /* Initialize global IDs */
  id_call    = rb_intern("call");
  id_inspect = rb_intern("inspect");
  id_to_f    = rb_intern("to_f");
  id_to_i    = rb_intern("to_i");
  id_to_s    = rb_intern("to_s");
  id_xop     = rb_intern("xop");

  mPortal = rb_define_module ("Portal");

  ePortalError = rb_define_class_under(mPortal, "Error", rb_eStandardError);

  PIN_ERR_SET_PROGRAM(portal_to_char(rb_gv_get("$PROGRAM_NAME")));

  /*
   * Module Portal
   */
  rb_define_module_function(mPortal,"field_num", portal_field_num,1);
  rb_define_module_function(mPortal,"field_type", portal_field_type,1);
  rb_define_module_function(mPortal,"field_name", portal_field_name,1);
  rb_define_module_function(mPortal,"set_program_name", portal_set_program_name,1);
  rb_define_module_function(mPortal,"set_log_level", portal_set_log_level,1);
  rb_define_module_function(mPortal,"hash_to_flist_string", portal_hash_to_flist_string,1);

#ifdef _DEBUG
  rb_define_module_function(mPortal,"test_flist_to_hash", portal_test_flist_to_hash,1);
#endif

  /*
   * Portal::Connection
   */
  cContext = rb_define_class_under(mPortal,"Connection", rb_cObject);
  rb_define_alloc_func(cContext, portal_alloc);
  rb_define_method(cContext, "initialize", portal_initialize, 0);
  rb_define_method(cContext, "connect", portal_connect, 1);
  rb_define_method(cContext, "disconnect", portal_disconnect, 1);
  rb_define_method(cContext, "loopback", portal_loopback, 0);
  rb_define_method(cContext, "session", portal_get_session, 0);
  rb_define_method(cContext, "userid", portal_get_userid, 0);
  rb_define_method(cContext, "inspectx", portal_inspect, 0);
  rb_define_method(cContext, "robj", portal_robj, -1);
  rb_define_method(cContext, "xop", portal_xop, -1);

}
