Summary:	Flexible HTML templating system
Name:		scat
Version:	0.03
Release:	3
License:	GPL
Group:		Applications/Publishing
Source0:	http://shellcat.sf.net/download/scat-%version.tar.gz
URL:		http://shellcat.sf.net/
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%description
Flexible HTML templating system.

%prep
%setup -q -n %{name}-%{version} 

%build
%configure --with-lang=en
make

%install
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT%{_bindir}
install scat $RPM_BUILD_ROOT%{_bindir}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%attr(755,root,root) %{_bindir}/* 
