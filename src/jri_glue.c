#include <R.h>
#include <Rinternals.h>
#include "rJava.h"

/* from Rglue.c - we could do without it ... */
SEXP j2SEXP(JNIEnv *env, jobject o, int releaseLocal);

/* creates a reference to an R object on the Java side 
   1) lock down the object in R
   2) call new Rengine(eng,robj)
 */
SEXP PushToREXP(SEXP clname, SEXP eng, SEXP engCl, SEXP robj) {
  char sig[128];
  jvalue jpar[4];
  jobject o;
  JNIEnv *env=getJNIEnv();
  char *cName;
  
  if (!isString(clname) || LENGTH(clname)!=1) error("invalid class name");
  if (!isString(engCl) || LENGTH(engCl)!=1) error("invalid engine class name");
  if (TYPEOF(eng)!=EXTPTRSXP) error("invalid engine object");
  R_PreserveObject(robj);
  sig[127]=0;
  cName = CHAR(STRING_ELT(clname,0));
  snprintf(sig,127,"(L%s;J)V",CHAR(STRING_ELT(engCl,0)));
  jpar[0].l = (jobject)EXTPTR_PTR(eng);
  jpar[1].j = (jlong) (unsigned long) robj;
  o = createObject(env, cName, sig, jpar, 1);
  if (!o) error("Unable to create Java object");
  return j2SEXP(env, o, 1);
  /* ok, some thoughts on mem mgmt - j2SEXP registers a finalizer. But I believe that is ok, because the pushed reference is useless until it is passed as an argument to some Java method. And then, it will have another reference which will prevent the Java side from being collected. The R-side reference may be gone, but that's ok, because it's the Java finalizer that needs to clean up the pushed R object and for that it doesn't need the proxy object at all. Thi sis the reason why RReleaseREXP uses EXTPTR - all the JAva finalizaer has to do is to call RReleaseREXP(self). For that it can create a fresh proxy object containing the REXP. But here comes he crux - this proxy cannot again create a reference - it must be plain pass-through, so this part needs to be verified. */
}

/* this is pretty much hard-coded for now - it's picking "xp" attribute */
SEXP RReleaseREXP(SEXP ptr) {
  jobject o;
  if (TYPEOF(ptr)==EXTPTRSXP) error("invalid object");
  o = (jobject)EXTPTR_PTR(ptr);
  {
    JNIEnv *env=getJNIEnv();
    jclass cls = (*env)->GetObjectClass(env, o);
    if (cls) {
      jfieldID fid=(*env)->GetFieldID(env,cls,"xp","J");
      if (fid) {
	jlong r=(*env)->GetLongField(env, o, fid);
	SEXP x = (SEXP)(unsigned long)r;
	if (x) R_ReleaseObject(x);
      }
    }
  }
  return R_NilValue;
}

    
