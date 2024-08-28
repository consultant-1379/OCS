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
Summary:   Installation package for FTPC Service.
Version:   %{_prNr}
Release:   %{_rel}
License:   Ericsson Proprietary
Vendor:    Ericsson LM
Packager:  %packer
Group:     IPNA
BuildRoot: %_tmppath
AutoReqProv: no

%define ipna_inetbin_path %{_cxcdir}
%define ipna_inetbin_bin %{ipna_inetbin_path}/bin
%define ipna_inetbin_conf %{ipna_inetbin_path}/conf
%define install_dir /data/apz/data
%define ipna_inet_bin ipna_inet.babsc.r1g01
%define ipna_inet_bin_old ipna_inet.babsc.r1g01
%define tmp_dir /tmp
%define tmp_dir_conf /tmp/ipna_inet_conf

%description
Installation package for IPNA_INET Service.

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
if [ ! -d $RPM_BUILD_ROOT%{tmp_dir_conf} ]
then
    mkdir -p $RPM_BUILD_ROOT%{tmp_dir_conf}
fi
echo "This is the %{_name} package %{_rel} install script"
cp %{ipna_inetbin_bin}/%{ipna_inet_bin} $RPM_BUILD_ROOT%{tmp_dir}/%{ipna_inet_bin}
cp %{ipna_inetbin_conf}/boot.ipnX.cp_loading $RPM_BUILD_ROOT%{tmp_dir_conf}/boot.ipn0
cp %{ipna_inetbin_conf}/boot.ipnX.cp_loading $RPM_BUILD_ROOT%{tmp_dir_conf}/boot.ipn1
cp %{ipna_inetbin_conf}/boot.ipnX.not_loading $RPM_BUILD_ROOT%{tmp_dir_conf}/boot.ipn2
cp %{ipna_inetbin_conf}/boot.ipnX.not_loading $RPM_BUILD_ROOT%{tmp_dir_conf}/boot.ipn3

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
if [ -d %{install_dir} ]
then
    cp -f $RPM_BUILD_ROOT%{tmp_dir}/%{ipna_inet_bin} %{install_dir}/%{ipna_inet_bin}

    chmod +x %{install_dir}/%{ipna_inet_bin}

    ln -sf %{install_dir}/%{ipna_inet_bin} %{_bindir}/%{ipna_inet_bin}
    cp %{tmp_dir_conf}/boot.ipn0 %{install_dir}/boot.ipn0
    cp %{tmp_dir_conf}/boot.ipn1 %{install_dir}/boot.ipn1
    cp %{tmp_dir_conf}/boot.ipn2 %{install_dir}/boot.ipn2
    cp %{tmp_dir_conf}/boot.ipn3 %{install_dir}/boot.ipn3

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
    rm -f %{_bindir}/%{ipna_inet_bin}
    rm -f %{install_dir}/%{ipna_inet_bin}
    rm -rf %{tmp_dir_conf}
fi

if [ $1 == 1 ]
then
    echo "This is the %{_name} package %{_rel} postun script during upgrade phase"
    rm -f %{_bindir}/%{ipna_inet_bin_old}
    rm -f %{tmp_dir}/%{ipna_inet_bin_old}
    rm -f %{install_dir}/%{ipna_inet_bin_old}
fi

%files
%defattr(-,root,root)
%{tmp_dir}/%{ipna_inet_bin}
%config %{tmp_dir_conf}/boot.ipn0
%config %{tmp_dir_conf}/boot.ipn1
%config %{tmp_dir_conf}/boot.ipn2
%config %{tmp_dir_conf}/boot.ipn3

%changelog
* Thu Nov 09 2011 - tu.h.do (at) dektech.com.au
- Create this spec file

