#include "tc_data_defs.h"
/*----------------------------------------------------------------------*/
/*
  INITIALIZATION SUBROUTINES
*/
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_ini3( TC_STRING database_path,
                                     TC_STRING temp_path,
                                     TC_INT nwsg,
                                     TC_INT nwse,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_ini( TC_INT nwsg,
                                    TC_INT nwse,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_relic();
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_sio( TC_STRING option,
                                    TC_INT ival);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gio( TC_STRING option,
                                    TC_INT* ival);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_rfil( TC_STRING file,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_sfil( TC_STRING file,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_ssu( TC_STRING quant,
                                    TC_STRING unit,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gsu( TC_STRING quant,
                                    TC_STRING unit,
                                    TC_STRING_LENGTH strlen_unit,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_same( TC_INT* icode,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
/*
  SYSTEM SUBROUTINES
*/
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_scom( TC_INT num,
                                     tc_components_strings* components,
                                     TC_FLOAT* stoi,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gcom( TC_INT* num,
                                     tc_components_strings* components,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gsci( TC_INT* index,
                                     TC_STRING component,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gnc( TC_INT* ncmp,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gnp( TC_INT* nph,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gpn( TC_INT index,
                                    TC_STRING phase,
                                    TC_STRING_LENGTH strlen_phase,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gpi( TC_INT* index,
                                    TC_STRING phase,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gpcn( TC_INT indexp,
                                     TC_INT indexc,
                                     TC_STRING name,
                                     TC_STRING_LENGTH strlen_name,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gpci( TC_INT indexp,
                                     TC_INT* indexc,
                                    TC_STRING name,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gccf( TC_INT indexc,
                                     TC_INT* nel,
                                     tc_elements_strings* elname,
                                     TC_FLOAT* stoi,
                                     TC_FLOAT* mmass,
                                     TC_INT* iwsg, TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gpcs( TC_INT indexp,
                                     TC_INT indexc,
                                     TC_FLOAT* stoi,
                                     TC_FLOAT* mmass,
                                     TC_INT* iwsg, TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gnpc( TC_INT indexp,
                                     TC_INT* npcon,
                                     TC_INT* iwsg, TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_cssc( TC_INT index,
                                     TC_STRING status,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    TC_BOOL     tq_gssc( TC_INT index,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_csp( TC_INT index,
                                    TC_STRING status,
                                    TC_FLOAT amount,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    TC_BOOL     tq_gsp( TC_INT index,
                                    TC_STRING status,
                                    TC_STRING_LENGTH strlen_status,
                                    TC_FLOAT* amount,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_setr( TC_INT indexc,
                                     TC_INT indexp,
                                     TC_FLOAT temp,
                                     TC_FLOAT press,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_sga( TC_INT indexp,
                                    TC_FLOAT value,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gga( TC_INT indexp,
                                    TC_FLOAT* value,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
/*
  CONDITION, STREAM, AND SEGMENT SUBROUTINES
*/
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_setc( TC_STRING condition,
                                     TC_INT i1,
                                     TC_INT i2,
                                     TC_FLOAT value,
                                     TC_INT* icond,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_remc( TC_INT index,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_scurc( TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_remac( TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_restc( TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_cstm( TC_STRING stream,
                                     TC_FLOAT temp,
                                     TC_FLOAT press,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_ssc( TC_STRING stream,
                                    TC_INT iph,
                                    TC_INT icmp,
                                    TC_FLOAT value,
                                    TC_INT icond,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_ssic( TC_STRING stavar,
                                     TC_FLOAT value,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_dstm(TC_STRING stream, 
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_nseg(TC_STRING id, 
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_sseg(TC_STRING id, 
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
/*
  CALCULATION AND RESULTS SUBROUTINES
*/
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_ce( TC_STRING var,
                                     TC_INT i1,
                                     TC_INT i2,
                                     TC_FLOAT value,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_ceg( TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_getv( TC_STRING var,
                                     TC_INT indexp,
                                     TC_INT indexc,
                                     TC_INT nvals,
                                     TC_FLOAT* values,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_get1( TC_STRING var,
                                     TC_INT indexp,
                                     TC_INT indexc,
                                     TC_FLOAT* value,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    TC_FLOAT    tq_gmu(  TC_INT icmp,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    TC_FLOAT    tq_ggm(  TC_INT iph,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gpd( TC_INT indexp,
                                    TC_INT* nsub,
                                    TC_INT* nscon,
                                    TC_FLOAT* sites,
                                    TC_FLOAT* yfrac,
                                    TC_FLOAT* extra,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gdf( TC_INT imatr,
                                    TC_INT iprec,
                                    TC_INT nph,
                                    TC_INT ncom,
                                    TC_FLOAT* xmatr,
                                    TC_FLOAT* xprec,
                                    TC_FLOAT* temp,
                                    TC_FLOAT* df,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gdf2(TC_INT mode,
                                    TC_INT imatr,
                                    TC_INT iprec,
                                    TC_INT nie,
                                    TC_INT *iie,
                                    TC_FLOAT* xmatr,
                                    TC_FLOAT temp,
                                    TC_FLOAT* df,
                                    TC_FLOAT* xprec,
                                    TC_FLOAT* xem,
                                    TC_FLOAT* xep,
                                    TC_FLOAT* mui,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
/*
  TROUBLESHOOTING SUBROUTINES
*/
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_ls( TC_INT* iwsg,
                                   TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_lc( TC_INT* iwsg,
                                   TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_le( TC_INT* iwsg,
                                   TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_fasv( TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_sdmc( TC_INT indexp,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_sspc( TC_INT indexp,
                                     TC_FLOAT* yf,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_ssv( TC_STRING stavar,
                                    TC_INT ip,
                                    TC_INT ic,
                                    TC_FLOAT value,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_pini( TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_snl( TC_INT maxit,
                                    TC_FLOAT acc,
                                    TC_FLOAT ymin,
                                    TC_FLOAT adg,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_smng( TC_INT ngp,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_seco( TC_INT ipdh,
				     TC_INT icss,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_st1err( TC_INT ierr,
                                       TC_STRING subr,
                                       TC_STRING mess);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_st2err( TC_INT ierr,
                                       TC_STRING subr,
                                       TC_STRING mess);
/*----------------------------------------------------------------------*/
TCFuncExport    TC_BOOL     tq_sg1err( TC_INT* ierr);
/*----------------------------------------------------------------------*/
TCFuncExport    TC_BOOL     tq_sg2err( TC_INT* ierr);
/*----------------------------------------------------------------------*/
TCFuncExport    TC_BOOL     tq_sg3err( TC_INT* ierr,
                                       TC_STRING subr,
                                       TC_STRING_LENGTH strlen_subr,
                                       TC_STRING mess,
                                       TC_STRING_LENGTH strlen_mess);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_reserr( );
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_sp3f( TC_STRING filename,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gfmm( TC_INT* freemem);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gver( TC_STRING version,
				     TC_STRING_LENGTH strlen_version,
				     TC_STRING lnkdat,
				     TC_STRING_LENGTH strlen_lnkdat,
				     TC_STRING osname,
				     TC_STRING_LENGTH strlen_osname,
				     TC_STRING build,
				     TC_STRING_LENGTH strlen_build,
				     TC_STRING cmpler,
				     TC_STRING_LENGTH strlen_cmpler);
/*----------------------------------------------------------------------*/
/*
  EXTRA SUBROUTINES
*/
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gma( TC_INT indexp,
                                    TC_FLOAT* tp,
                                    TC_FLOAT* yf,
                                    TC_FLOAT* varr,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gmb( TC_INT indexp,
                                    TC_FLOAT* tp,
                                    TC_FLOAT* varr,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gmc( TC_INT indexp,
                                    TC_FLOAT* varr,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gmdy( TC_INT indexp,
                                     TC_FLOAT* varr,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gmob( TC_INT indexp,
                                     TC_INT isp,
                                     TC_FLOAT* val,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_stp( TC_FLOAT* tp,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_syf( TC_INT indexp,
                                    TC_FLOAT* yf,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gsspi( TC_STRING name,
                                      TC_INT* index,
                                      TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    TC_BOOL     tq_cmoba( TC_INT indexp,
                                      TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    TC_BOOL     tq_cmobb( TC_INT indexp,
                                      TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_dgyy( TC_INT indexp,
                                     TC_FLOAT* varr1,
                                     TC_FLOAT* varr2,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gphp( TC_INT indexp,
                                     TC_INT* ne,
                                     TC_INT* ncnv,
                                     TC_INT* nc,
                                     TC_INT* iwork,
                                     TC_FLOAT* work,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_x2y( TC_INT indexp,
                                    TC_INT ne,
                                    TC_INT ncnv,
                                    TC_INT nc,
                                    TC_INT* iwork,
                                    TC_FLOAT* work,
                                    TC_FLOAT* xf,
                                    TC_FLOAT* yf,
                                    TC_INT* iwsg,
                                    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    TC_FLOAT    tq_gse( TC_INT ipm,
				    TC_INT ipp,
				    TC_INT imc,
				    TC_FLOAT t,
				    TC_FLOAT* x,
				    TC_FLOAT vm,
				    TC_FLOAT vp,
				    TC_INT* iwsg,
				    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gmdx( TC_INT indexp,
                                     TC_INT ne,
                                     TC_INT ncnv,
                                     TC_INT nc,
                                     TC_INT* iwork,
                                     TC_FLOAT* work,
                                     TC_FLOAT* yf,
                                     TC_FLOAT* varr,
                                     TC_FLOAT* gm,
                                     TC_FLOAT* dgdx,
                                     TC_FLOAT* xf,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_syscpu( TC_INT code,
				       TC_INT* val);
/*----------------------------------------------------------------------*/
/*
  DATABASE SUBROUTINES
*/
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gdbn( tc_databases_strings* databases,
                                     TC_INT* n,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_opdb( TC_STRING database,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_lide( tc_elements_strings* elements,
                                     TC_INT* num,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_apdb( TC_STRING database,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_defel( TC_STRING element,
                                      TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_rejel( TC_STRING element,
                                      TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_lisph( tc_phases_strings* phases,
                                     TC_INT* num,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_lissf( tc_phases_strings* phases,
                                     TC_INT* num,
                                     TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_resph( TC_STRING phase,
                                      TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_rejph( TC_STRING phase,
                                      TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gdat( TC_INT* iwsg,
                                     TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_rejsy( TC_INT* iwsg,
                                      TC_INT* iwse);
/*----------------------------------------------------------------------*/
/*
  INTERPOLATION SCHEME
*/
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_qphasw( TC_INT iph,
                                       TC_INT icsc,
                                       TC_STRING name,
                                       TC_STRING_LENGTH strlen_name,
                                       TC_INT* ipos,
                                       TC_INT* iwse,
                                       TC_INT* iwsg);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_gspha(  TC_STRING name,
                                       TC_INT* iph,
                                       TC_INT* iwsg);
/*----------------------------------------------------------------------*/
TCFuncExport    void        tq_winit(  TC_INT nws,
                                       TC_INT ires,
                                       TC_INT* iwsg);
/*----------------------------------------------------------------------*/
TCFuncExport    void    tq_ips_init_top( TC_INT* err,
					 TC_INT* iwsg,
					 TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void    tq_ips_init_branch( TC_BOOL t_is_condition,
					    TC_BOOL t_is_constant,
					    TC_BOOL p_is_condition,
					    TC_BOOL p_is_constant,
					    TC_BOOL* independent_elements,
					    TC_INT dicretization_type,
					    TC_INT nr_of_steps,
					    TC_INT* state_of_phases,
					    TC_FLOAT t,
					    TC_FLOAT tmin,
					    TC_FLOAT tmax,
					    TC_FLOAT p,
					    TC_FLOAT pmin,
					    TC_FLOAT pmax,
					    TC_FLOAT memory_fraction,
					    TC_FLOAT* amount_of_phases,
					    TC_FLOAT* xmin,
					    TC_FLOAT* xmax,
					    TC_INT* branch_nr,
					    TC_INT* err,
					    TC_INT* iwsg,
					    TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void    tq_ips_init_function( TC_STRING function_string,
					      TC_INT branch_nr,
					      TC_INT* err,
					      TC_INT* iwsg,
					      TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void    tq_ips_get_value( TC_INT branch_nr,
					  TC_INT noscheme,
					  TC_FLOAT* variable_values,
					  TC_FLOAT* function_values,
					  TC_INT* err,
					  TC_INT* shortcut,
					  TC_INT* iwsg,
					  TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void    tq_ips_set_optimization(TC_INT istat,
						TC_INT* ierr,
						TC_INT* iwsg,
						TC_INT* iwse);
/*----------------------------------------------------------------------*/
/*
  TQ-REORDER PHASES
*/
/*----------------------------------------------------------------------*/
TCFuncExport    void 
tq_pacs( TC_INT index,
	 TC_INT* iwsg,
	 TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void 
tq_roinit( TC_INT nwsr,
	   TC_INT* iwsr,
	   TC_INT* iwsg,
	   TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void 
tq_setrx( TC_STRING phase,
	  TC_FLOAT* x,
	  TC_INT* iwsr,
	  TC_INT* iwsg,
	  TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void 
tq_order( TC_INT* iwsr,
	  TC_INT* iwsg,
	  TC_INT* iwse);
/*----------------------------------------------------------------------*/
TCFuncExport    void 
tq_lrox( TC_INT* iwsr,
	 TC_INT* iwsg,
	 TC_INT* iwse);
/*----------------------------------------------------------------------*/
