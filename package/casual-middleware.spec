#
# Spec for casual
#
Summary: Casual Middleware
Name: casual-middleware
Version: %{casual_version}
Release: %{casual_release}
License: GPL
Group: Applications/System
Source: -
URL: https://bitbucket.org/casualcore/casual
Distribution: Centos

Prefix: /opt/casual

%description
This is the core package for casual-middleware

%prep
%build
%install
install -m 0755 -d $RPM_BUILD_ROOT/opt/casual
cp -r /opt/casual/* $RPM_BUILD_ROOT/opt/casual/.
find $RPM_BUILD_ROOT/opt/casual | xargs chmod 0755

%files
/opt/casual

%changelog
* Tue Sep  22 2015  Fredrik Eriksson <lazan@laz.se> 
- Some insightful comment

