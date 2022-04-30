#
# Spec for casual
#
Summary: Casual Middleware
Name: casual-middleware
Version: %{casual_version}
Release: %{casual_release}%{?dist}
License: MIT
Group: Applications/System
Source: -
URL: https://bitbucket.org/casualcore/casual
Distribution: %{distribution}

Prefix: /opt/casual

%description
This is the core package for casual-middleware

%prep
%build
%install
install -m 0755 -d $RPM_BUILD_ROOT/opt/casual
cp -r ${CASUAL_HOME}/* $RPM_BUILD_ROOT/opt/casual/.
find $RPM_BUILD_ROOT/opt/casual | xargs chmod 0755
install -m 0755 -d $RPM_BUILD_ROOT/etc/bash_completion.d/
ln -sf ${CASUAL_HOME}/etc/bash_completion.d/casual $RPM_BUILD_ROOT/etc/bash_completion.d/casual
ln -sf ${CASUAL_HOME}/etc/bash_completion.d/casual-log $RPM_BUILD_ROOT/etc/bash_completion.d/casual-log
ln -sf ${CASUAL_HOME}/include/casual/buffer $RPM_BUILD_ROOT/opt/casual/include/buffer
install -m 0755 -d $RPM_BUILD_ROOT/etc/casual/
install -m 0755 ${CASUAL_HOME}/configuration/example/resources.yaml $RPM_BUILD_ROOT/etc/casual/

%files
/opt/casual
/etc/bash_completion.d/casual
/etc/bash_completion.d/casual-log

%config /etc/casual/resources.yaml

%changelog
* Tue Sep  22 2015  Fredrik Eriksson <lazan@laz.se> 
- Some insightful comment

