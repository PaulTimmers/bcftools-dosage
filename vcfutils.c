#include "vcfutils.h"

int calc_ac(const bcf_hdr_t *header, bcf1_t *line, int *ac, int which)
{
	int i;
	for (i=0; i<line->n_allele; i++) ac[i]=0;

	// Use INFO/AC,AN field only when asked
	if ( which&BCF_UN_INFO )
	{
		bcf_unpack(line, BCF_UN_INFO);
		int an_id = bcf_id2int(header, BCF_DT_ID, "AN");
		int ac_id = bcf_id2int(header, BCF_DT_ID, "AC");
		if ( an_id>=0 && ac_id>=0 )
		{
			int i, an=0, ac_len=0;
			uint8_t *ac_ptr=NULL;
			for (i=0; i<line->n_info; i++)
			{
				bcf_info_t *z = &line->d.info[i];
				if ( z->key == an_id ) an = z->v1.i;
				else if ( z->key == ac_id ) { ac_ptr = z->vptr; ac_len = z->len; }
			}
			int nac = 0;
			for (i=0; i<ac_len; i++)
			{
				ac[i+1] = ac_ptr[i]; 
				nac += ac_ptr[i];
			}
			ac[0] = an - nac;
			return 1;
		}
	}

	// Split genotype fields only when asked
	if ( which&BCF_UN_FMT )
	{
		int i, gt_id = bcf_id2int(header,BCF_DT_ID,"GT");
		if ( gt_id<0 ) return 0;
		bcf_unpack(line, BCF_UN_FMT);
		bcf_fmt_t *gt = NULL;
		for (i=0; i<(int)line->n_fmt; i++) 
			if ( line->d.fmt[i].id==gt_id ) { gt = &line->d.fmt[i]; break; }
		if ( !gt ) return 0;
		uint8_t *p = gt->p;
		for (i=0; i<line->n_sample; i++)
		{
			int ial;
			for (ial=0; ial<gt->size; ial++)
			{
				if ( *p ) ac[((*p)>>1)-1]++;
				p++;
			}
		}
		return 1;
	}

	return 0;
}

inline int gt_type(bcf_fmt_t *fmt_ptr, int isample)
{
	int i;
	char gt = GT_HOM_RR;
	uint8_t *p = &fmt_ptr->p[isample*fmt_ptr->size];
	for (i=1; i<fmt_ptr->size; i++)
	{
		if ( p[0]>>1 != p[i]>>1 ) 
		{ 
			gt = GT_HET_RA; 
			break; 
		}
	}
	if ( gt==GT_HOM_RR && (p[0]>>1)>1 ) return GT_HOM_AA;
	return gt;
}

