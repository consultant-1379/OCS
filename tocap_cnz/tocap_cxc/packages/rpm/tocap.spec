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
Summary:   Installation package for TOCAP Service.
Version:   %{_prNr}
Release:   %{_rel}
License:   Ericsson Proprietary
Vendor:    Ericsson LM
Packager:  %packer
Group:     OCS
BuildRoot: %_tmppath
AutoReqProv: no
Requires: APOS_OSCONFBIN

%define tocap_cxc_path %{_cxcdir}
%define tocap_cxc_bin %{tocap_cxc_path}/bin
%define tocap_cxc_conf %{tocap_cxc_path}/conf
%define ocs_data_dir %{OCSdir}/data

%description
Installation package for TOCAP Service.

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

cp %{tocap_cxc_bin}/ocs_tocapd $RPM_BUILD_ROOT%{OCSBINdir}/ocs_tocapd
cp %{tocap_cxc_bin}/ocs_tocap_tocapservice_clc $RPM_BUILD_ROOT%{OCSBINdir}/ocs_tocap_tocapservice_clc

%post
if [ $1 == 1 ] 
then
echo "This is the %{_name} package %{_rel} post script during installation phase"

ln -sf $RPM_BUILD_ROOT%{OCSBINdir}/ocs_tocapd $RPM_BUILD_ROOT/%{_bindir}/ocs_tocapd

# Create ocs data dir in local disk
if [ ! -d $RPM_BUILD_ROOT%{ocs_data_dir} ]
then 
    mkdir -p $RPM_BUILD_ROOT%{ocs_data_dir}
fi
fi

if [ $1 == 2 ]
then
echo "This is the %{_name} package %{_rel} post script during upgrade phase"
fi

# For both post phases
chmod +x $RPM_BUILD_ROOT%{OCSBINdir}/ocs_tocapd
chmod +x $RPM_BUILD_ROOT%{OCSBINdir}/ocs_tocap_tocapservice_clc

# Remove /opt/ap/ocs/conf/tocap dir if exist. Fix TR HO54346
if [ -d $RPM_BUILD_ROOT%{OCSCONFdir}/tocap ]
then
    rm -r $RPM_BUILD_ROOT%{OCSCONFdir}/tocap
fi

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
rm -f %{_bindir}/ocs_tocapd
fi

if [ $1 == 1 ]
then
echo "This is the %{_name} package %{_rel} postun script during upgrade phase"
fi

%files
%defattr(-,root,root)
%{OCSBINdir}/ocs_tocapd
%{OCSBINdir}/ocs_tocap_tocapservice_clc

%changelog
* Wed Oct 02 2012 - tuan.nguyen (at) dektech.com.au
- remove startTOCAP
* Wed Apr 11 2012 - tuan.nguyen (at) dektech.com.au
- Remove ha_ocs_tocap_objects.xml from rpm
* Thu Jul 07 2011 - tuan.nguyen (at) dektech.com.au
- Update ha_ocs_tocap_objects.xml location
* Wed Jul 06 2011 - tuan.nguyen (at) dektech.com.au
- Update for upgrade instalation
* Thu Feb 09 2011 - chinh.hoang (at) dektech.com.au
- Update with label SDK_APG43L_PROJ_0504_002
* Mon Jul 01 2010 - giovanni.gambardella (at) ericsson.com
- Initial implementation

