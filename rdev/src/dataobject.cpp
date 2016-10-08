
// This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
// To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/ or send a letter to
// Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

#include "RcppInclude.h"
#include "dataobject.h"
#include "mrgsolve.h"
#include "mrgsolv.h"

#define _COL_amt_   0u
#define _COL_ii_    1u
#define _COL_addl_  2u
#define _COL_ss_    3u
#define _COL_rate_  4u
#define _COL_evid_  5u
#define _COL_cmt_   6u
#define _COL_time_  7u


dataobject::dataobject(Rcpp::NumericMatrix _data, 
                       Rcpp::CharacterVector _parnames) {
  Data = _data;
  parnames = _parnames;
  
  Rcpp::List dimnames = Data.attr("dimnames");
  Data_names = Rcpp::as<Rcpp::CharacterVector>(dimnames[1]);
  
  Idcol = find_position("ID", Data_names);
  if(Idcol < 0) Rcpp::stop("Could not find ID column in data set.");
  
  // Connect Names in the data set with positions in the parameter list
  from_to(Data_names,parnames, par_from, par_to);
  
  col.resize(8,0);
  
}

dataobject::dataobject(Rcpp::NumericMatrix _data,
                       Rcpp::CharacterVector _parnames,
                       Rcpp::CharacterVector _cmtnames) {
  Data = _data;
  parnames = _parnames;
  cmtnames = _cmtnames;
  
  Rcpp::List dimnames = Data.attr("dimnames");
  Data_names = Rcpp::as<Rcpp::CharacterVector>(dimnames[1]);
  
  Idcol = find_position("ID", Data_names);
  
  if(Idcol < 0) Rcpp::stop("Could not find ID column in data set.");
  
  // Connect Names in the data set with positions in the parameter list
  from_to(Data_names, parnames, par_from, par_to);
  from_to(Data_names, cmtnames, cmt_from, cmt_to);
  
  col.resize(8,0);
  
}


dataobject::~dataobject(){}

void dataobject::map_uid() {
  
  int i=0;
  int n = Data.nrow();
  
  Uid.push_back(Data(0,Idcol));
  Startrow.push_back(0);
  
  for(i=1; i < n; ++i) {
    if(Data(i-1,Idcol) != Data(i, Idcol)) {
      Uid.push_back(Data(i,Idcol));
      Startrow.push_back(i);
      Endrow.push_back(i-1);
    }
  }
  Endrow.push_back(n-1);
}


Rcpp::IntegerVector dataobject::get_col_n(const Rcpp::CharacterVector& what) {
  Rcpp::IntegerVector ret = Rcpp::match(what, Data_names);
  ret = Rcpp::na_omit(ret);
  return(ret-1);
}


void dataobject::locate_tran() {
  
  
  unsigned int zeros = Data.ncol()-1;
  
  if(zeros==0) {
    col[_COL_amt_]  = 0;
    col[_COL_ii_]   = 0;
    col[_COL_addl_] = 0;
    col[_COL_ss_]   = 0;
    col[_COL_rate_] = 0;
    col[_COL_evid_] = 0;
    col[_COL_cmt_]  = 0;
    col[_COL_time_] = 0;
    return;
  }
  
  Rcpp::CharacterVector::iterator bg = Data_names.begin();
  Rcpp::CharacterVector::iterator ed = Data_names.end();
  
  unsigned int tcol = std::find(bg,ed,"time") - bg;
  
  bool lc = true;
  
  if(tcol > zeros) {
    tcol = std::find(bg,ed,"TIME") - bg;
    if(tcol > zeros) Rcpp::stop("Could not find time or TIME column in the data set.");
    lc = false;
  }
  
  col[_COL_time_] = tcol;
  
  if(lc) {
    col[_COL_amt_]  = std::find(bg,ed,"amt")  - bg;
    col[_COL_ii_]   = std::find(bg,ed,"ii")   - bg;
    col[_COL_addl_] = std::find(bg,ed,"addl") - bg;
    col[_COL_ss_]   = std::find(bg,ed,"ss")   - bg;
    col[_COL_rate_] = std::find(bg,ed,"rate") - bg;
    col[_COL_evid_] = std::find(bg,ed,"evid") - bg;
    col[_COL_cmt_]  = std::find(bg,ed,"cmt")  - bg;
    
  } else {
    col[_COL_amt_]  = std::find(bg,ed,"AMT")  - bg;
    col[_COL_ii_]   = std::find(bg,ed,"II")   - bg;
    col[_COL_addl_] = std::find(bg,ed,"ADDL") - bg;
    col[_COL_ss_]   = std::find(bg,ed,"SS")   - bg;
    col[_COL_rate_] = std::find(bg,ed,"RATE") - bg;
    col[_COL_evid_] = std::find(bg,ed,"EVID") - bg;
    col[_COL_cmt_]  = std::find(bg,ed,"CMT")  - bg;
  }
  
  if(col[_COL_amt_]  > zeros) col[_COL_amt_]  = zeros;
  if(col[_COL_ii_]   > zeros) col[_COL_ii_]   = zeros;
  if(col[_COL_addl_] > zeros) col[_COL_addl_] = zeros;
  if(col[_COL_ss_]   > zeros) col[_COL_ss_]   = zeros;
  if(col[_COL_rate_] > zeros) col[_COL_rate_] = zeros;
  if(col[_COL_evid_] > zeros) col[_COL_evid_] = zeros;
  
  if(col[_COL_cmt_] > zeros  && zeros > 0) {
    Rcpp::stop("Couldn't locate cmt or CMT in data set.");
  }
}

void dataobject::idata_row() {
  for(int i=0; i < Data.nrow(); ++i) {
    idmap[Data(i,Idcol)] = i;
  }
}

void dataobject::copy_parameters(int this_row, odeproblem *prob) {
  size_t n = par_from.size();
  for(size_t i=0; i < n; ++i) {
    prob->param(par_to[i],Data(this_row,par_from[i]));
  }
}


void dataobject::copy_inits(int this_row, odeproblem *prob) {
  // this should only be done from idata sets
  size_t n = cmt_from.size();
  for(size_t i=0; i < n; ++i) {
    prob->y_init(cmt_to[i],Data(this_row,cmt_from[i]));
  }
}


void dataobject::reload_parameters(const Rcpp::NumericVector& PARAM, 
                                   odeproblem *prob) {
  size_t n = par_to.size();
  for(size_t i = 0; i < n; ++i) {
    prob->param(par_to[i],PARAM[par_to[i]]);
  }
}



void dataobject::get_records(recstack& a, int NID, int neq,
                             unsigned int& obscount, unsigned int& evcount,
                             bool obsonly, bool debug) {
  
  // only look here for events or observations if there is more than one column:
  // size_t h=0; warnings
  int j=0;
  int this_cmt;
  double lastime = 0;
  
  if(Data.ncol() <= 1) {
    return;
  }
  
  for(int h=0; h < NID; ++h) {
    
    lastime = 0;
    
    a[h].reserve(this->end(h) - this->start(h) + 5);
    
    for(j = this->start(h); j <= this->end(h); ++j) {
      
      if(Data(j,col[_COL_time_]) < lastime) {
        Rcpp::stop("Problem with time: data set is not sorted by time or time is negative.");
      }
      
      lastime = Data(j,col[_COL_time_]);
      
      this_cmt = Data(j,col[_COL_cmt_]);
      
      // If this is an observation record
      if(Data(j,col[_COL_evid_])==0) {
        
        if((this_cmt < 0) || (this_cmt > neq)) {
          Rcpp::stop("cmt number in observation record out of range.");
        }
        
        rec_ptr obs = boost::make_shared<datarecord>(
          Data(j,col[_COL_time_]),
          Data(j,col[_COL_cmt_]),
          j,
          Data(j,Idcol)
        );
        
        a[h].push_back(obs);
        ++obscount;
        continue;
      }
      
      // Check that cmt is valid:
      if((this_cmt==0) || (abs(this_cmt) > neq)) {
        Rcpp::stop("cmt number in dosing record out of range.");
      }
      
      ++evcount;
      
      rec_ptr ev = boost::make_shared<datarecord>(
        Data(j,col[_COL_cmt_]),
        Data(j,col[_COL_evid_]),
        Data(j,col[_COL_amt_]),
        Data(j,col[_COL_time_]),
        Data(j,col[_COL_rate_]),
        j, 
        Data(j,Idcol)
      );
      
      if((ev->rate() < 0) && (ev->rate() != -2) && (ev->rate() != -1)) {
        Rcpp::stop("Non-zero rate must be positive or equal to -1 or -2");
      }
      
      if((ev->rate() != 0) && (ev->amt() <= 0) && (ev->evid()==1)) {
        Rcpp::stop("Non-zero rate requires positive amt.");
      }
      
      ev->from_data(true);
      if(!obsonly) ev->output(true);
      
      ev->ss(Data(j,col[_COL_ss_]));
      ev->addl(Data(j,col[_COL_addl_]));
      ev->ii(Data(j,col[_COL_ii_]));
      
      if(ev->ii() <= 0) {
        if(ev->addl() > 0) {
          Rcpp::stop("Found dosing record with addl > 0 and ii <= 0.");
        }
        if(ev->ss()) {
          Rcpp::stop("Found dosing record with ss==1 and ii <= 0.");
        }
      }
      
      a[h].push_back(ev);
      
    }
  }
}


void dataobject::get_ids(uidtype* ids) {
  for(int i = 0; i < Data.nrow(); ++i) {
    ids->push_back(Data(i,Idcol)); 
  }
}


void dataobject::check_idcol(dataobject& idat) {
  
  if(idat.ncol() == 0) {return;}
  
  uidtype uidata;
  idat.get_ids(&uidata);
  
  // Uids from data
  uidtype uthis  = this->return_uid();
  
  sort_unique(uthis);
  sort_unique(uidata);
  
  if(!std::includes(uidata.begin(),uidata.end(),uthis.begin(),uthis.end())) {
    Rcpp::stop("ID found in the data set, but not in idata.");
  }
}
