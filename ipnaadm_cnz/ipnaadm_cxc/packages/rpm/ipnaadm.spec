#
# spec file for configuration of package cdh service prototype
#
# Copyright  (c)  2010  Ericsson LM
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# please send bugfixes or comments to paolo.palmieri@ericsson.com
#                                     giovanni.gambardella@ericsson.com
#

Name:      %{_name}
Summary:   Installation package for IPNAADM.
Version:   %{_prNr}
Release:   %{_rel}
License:   Ericsson Proprietary
Vendor:    Ericsson LM
Packager:  %packer
Group:     OCS
BuildRoot: %_tmppath
AutoReqProv: no
Requires: APOS_OSCONFBIN

%define ipnaadm_cxc_path %{_cxcdir}
%define ipnaadm_cxc_bin %{ipnaadm_cxc_path}/bin
%define ipnaadm_cxc_conf %{ipnaadm_cxc_path}/conf

%description
Installation package for IPNAAADM.

%pre
if [ $1 == 1 ] 
then
echo "This is the %{_name} package %{_rel} pre-install script during installation phase"
fi
if [ $1 == 2 ]
then
echo "This is the %{_name} package %{_rel} pre-install script during upgrade phase"
fi

%install
echo "This is the %{_name} package %{_rel} install script"

if [ ! -d $RPM_BUILD_ROOT%{rootdir} ]
then
    mkdir $RPM_BUILD_ROOT%{rootdir}
fi
if [ ! -d $RPM_BUILD_ROOT%{APdir} ]
then
    mkdir $RPM_BUILD_ROOT%{APdir}
fi    
if [ ! -d $RPM_BUILD_ROOT%{OCSdir} ]
then
    mkdir $RPM_BUILD_ROOT%{OCSdir}
fi    
if [ ! -d $RPM_BUILD_ROOT%{OCSBINdir} ]
then
    mkdir $RPM_BUILD_ROOT%{OCSBINdir}
fi    
if [ ! -d $RPM_BUILD_ROOT%{OCSLIBdir} ]
then
    mkdir $RPM_BUILD_ROOT%{OCSLIBdir}
fi
if [ ! -d $RPM_BUILD_ROOT%{OCSETCdir} ]
then
    mkdir $RPM_BUILD_ROOT%{OCSETCdir}
fi
if [ ! -d $RPM_BUILD_ROOT%{OCSCONFdir} ]
then
    mkdir $RPM_BUILD_ROOT%{OCSCONFdir}
fi

cp %{ipnaadm_cxc_bin}/ocs_ipnaadmd $RPM_BUILD_ROOT%{OCSBINdir}/ocs_ipnaadmd
cp %{ipnaadm_cxc_bin}/ipnaadm $RPM_BUILD_ROOT%{OCSBINdir}/ipnaadm
cp %{ipnaadm_cxc_bin}/ipnaadm.sh $RPM_BUILD_ROOT%{OCSBINdir}/ipnaadm.sh
cp %{ipnaadm_cxc_bin}/ocs_ipnaadm_ipnaadmservice_clc $RPM_BUILD_ROOT%{OCSBINdir}/ocs_ipnaadm_ipnaadmservice_clc

%post
if [ $1 == 1 ] 
then
echo "This is the %{_name} package %{_rel} post script during installation phase"

fi

if [ $1 == 2 ]
then
echo "This is the %{_name} package %{_rel} post script during upgrade phase"
fi

# For both post phases
chmod +x $RPM_BUILD_ROOT%{OCSBINdir}/ocs_ipnaadmd
chmod +x $RPM_BUILD_ROOT%{OCSBINdir}/ipnaadm
chmod +x $RPM_BUILD_ROOT%{OCSBINdir}/ipnaadm.sh
chmod +x $RPM_BUILD_ROOT%{OCSBINdir}/ocs_ipnaadm_ipnaadmservice_clc

ln -sf $RPM_BUILD_ROOT%{OCSBINdir}/ocs_ipnaadmd $RPM_BUILD_ROOT/%{_bindir}/ocs_ipnaadmd
ln -sf $RPM_BUILD_ROOT%{OCSBINdir}/ipnaadm.sh $RPM_BUILD_ROOT/%{_bindir}/ipnaadm

%preun
if [ $1 == 0 ]
then
echo "This is the %{_name} package %{_rel} preun script during unistall phase"

fi

if [ $1 == 1 ]
then
echo "This is the %{_name} package %{_rel} preun script during upgrade phase"
fi

%postun
if [ $1 == 0 ]
then
echo "This is the %{_name} package %{_rel} postun script during unistall phase"
rm -f %{_bindir}/ocs_ipnaadmd
rm -f %{_bindir}/ipnaadm
fi

if [ $1 == 1 ]
then
echo "This is the %{_name} package %{_rel} postun script during upgrade phase"
fi

%files
%defattr(-,root,root)
%{OCSBINdir}/ocs_ipnaadmd
%{OCSBINdir}/ipnaadm
%{OCSBINdir}/ipnaadm.sh
%{OCSBINdir}/ocs_ipnaadm_ipnaadmservice_clc

%changelog
* Tue Oct 01 2012 - tuan.nguyen (at) dektech.com.au
- update ipnaadm.sh
- update APOS_OSCONFBIN 
* Wed Apr 11 2012 - tuan.nguyen (at) dektech.com.au
- remove ha_ocs_ipnaadm_ipnaadmservice_objects.xml file from rpm package
* Thu Oct 06 2011 - tuan.nguyen (at) dektech.com.au
- Update the amf scripts
* Thu Aug 22 2011 - tuan.nguyen (at) dektech.com.au
- Modify to IPNAADM rpm
* Mon Jul 01 2010 - giovanni.gambardella (at) ericsson.com
- Initial implementation

