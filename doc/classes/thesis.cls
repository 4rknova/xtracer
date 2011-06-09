% thesis v0.1
\ProvidesClass{thesis}[2011/04/01 v0.1 thesis class]

%-------------------------- initial code -----------------------
\newif\ifbookmode
\bookmodetrue

\DeclareOption{report}{\bookmodefalse}
\DeclareOption{book}{\bookmodetrue}
\ProcessOptions\relax%

\ifbookmode
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{book}}%
\ProcessOptions\relax%
\ifx\pdfoutput\undefined%
   \LoadClass[dvips, a4paper]{book}%
\else%
   \LoadClass[pdftex, a4paper]{book}%
\fi%
\else
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{report}}%
\ProcessOptions\relax%
\ifx\pdfoutput\undefined%
  \LoadClass[dvips, a4paper]{report}%
\else%
   \LoadClass[pdftex, a4paper]{report}%
\fi%
%\renewcommand{\refname}{References}%
\fi

\usepackage{setspace}
\usepackage{tocbibind}
\usepackage{amssymb}
\usepackage{fancyhdr}
\usepackage{eucal}
\usepackage[greek,english]{babel}
\usepackage[cm-default]{fontspec}
\setromanfont{FreeSerif}
\setsansfont{FreeSans}
\setmonofont{FreeMono}
\usepackage[iso-8859-7]{inputenc}
\usepackage[usenames, dvipsnames]{color}
\usepackage[perpage]{footmisc}
\usepackage[square, sort, numbers, authoryear]{natbib}
\usepackage{ifthen}
\usepackage{ifpdf}

\usepackage{color}
\usepackage{listings}
\renewcommand\lstlistingname{Πλαίσιο}
\renewcommand\lstlistlistingname{Πλαίσιο}

% Math-mode symbol & verbatim
\def\W#1#2{$#1{#2}$ &\tt\string#1\string{#2\string}}
\def\X#1{$#1$ &\tt\string#1}
\def\Y#1{$\big#1$ &\tt\string#1}
\def\Z#1{\tt\string#1}

%Bibliography
\renewcommand{\bibname}{Βιβλιογραφικές πηγές}

\ifpdf
    \usepackage[ pdftex, plainpages = false, pdfpagelabels, 
                 pdfpagelayout = useoutlines,
                 bookmarks,
                 bookmarksopen = true,
                 bookmarksnumbered = true,
                 breaklinks = true,
                 linktocpage,
                 pagebackref,
                 colorlinks = true,
                 linkcolor = blue,
                 urlcolor  = blue,
                 citecolor = red,
                 anchorcolor = green,
                 hyperindex = true,
                 hyperfigures
                 ]{hyperref} 
    \usepackage[pdftex]{graphicx}
    \DeclareGraphicsExtensions{.png, .jpg, .pdf}

    \pdfcompresslevel=9
    \graphicspath{{fig/}}
\else
    \usepackage[ dvips, 
                 bookmarks,
                 bookmarksopen = true,
                 bookmarksnumbered = true,
                 breaklinks = true,
                 linktocpage,
                 pagebackref,
                 colorlinks = true,
                 linkcolor = blue,
                 urlcolor  = blue,
                 citecolor = red,
                 anchorcolor = green,
                 hyperindex = true,
                 hyperfigures
                 ]{hyperref}

    %\usepackage{epsfig}
    \usepackage{graphicx}
    \DeclareGraphicsExtensions{.eps, .ps}
    \graphicspath{{fig/}}
\fi

%A4 settings
\ifpdf
   \pdfpageheight=297mm
   \pdfpagewidth=210mm
\else
   \setlength{\paperheight}{297mm}
   \setlength{\paperwidth}{210mm}
\fi

\setlength{\hoffset}{0.00cm}
\setlength{\voffset}{0.00cm}

\setlength{\evensidemargin}{1.96cm}
\setlength{\topmargin}{1mm}
\setlength{\headheight}{1.36cm}
\setlength{\headsep}{1.00cm}
\setlength{\textheight}{20.84cm}
\setlength{\textwidth}{14.5cm}
\setlength{\marginparsep}{1mm}
\setlength{\marginparwidth}{3cm}
\setlength{\footskip}{2.36cm}
\frenchspacing

\pagestyle{fancy}
\fancyhf{}
\renewcommand{\headrulewidth}{0pt}
\renewcommand{\chaptermark}[1]{\markboth{\MakeUppercase{\thechapter. #1 }}{}}
\renewcommand{\sectionmark}[1]{}
\fancyhead[LE,LO]{\@stitle}
\fancyhead[RO,RE]{\chaptername \, \thechapter}
\fancyfoot[C]{\thepage}
\renewcommand{\headrulewidth}{0.5pt}
\renewcommand{\footrulewidth}{0pt}
\addtolength{\headheight}{0.5pt}
\fancypagestyle{plain}{
  \fancyhead{}
  \renewcommand{\headrulewidth}{0pt}
}

\newcommand{\submittedtext}{{A thesis submitted for the degree of}}

% DECLARATIONS
%
% These macros are used to declare arguments needed for the
% construction of the title page and other preamble.

% Stitle
\def\stitle#1{\gdef\@stitle{#1}}

% The registration code
\def\regcode#1{\gdef\@regcode{#1}}

% The supervisor
\def\supervisor#1{\gdef\@supervisor{#1}}
\def\supervprof#1{\gdef\@supervprof{#1}}

% The year and term the degree will be officially conferred
\def\degreedate#1{\gdef\@degreedate{#1}}
% The full (unabbreviated) name of the degree
%\def\degree#1{\gdef\@degree{#1}}
% The name of your college or department(eg. Trinity, Pembroke, Maths, Physics)
\def\collegeordept#1{\gdef\@collegeordept{#1}}
% The name of your University
\def\university#1{\gdef\@university{#1}}
% Defining the crest
\def\crest#1{\gdef\@crest{#1}}

% These macros define an environment for front matter that is always 
% single column even in a double-column document.

\newenvironment{alwayssingle}{%
       \@restonecolfalse\if@twocolumn\@restonecoltrue\onecolumn
       \else\newpage\fi}
       {\if@restonecol\twocolumn\else\newpage\fi}

%define title page layout
\renewcommand{\maketitle}{%
\begin{alwayssingle}
    \renewcommand{\footnotesize}{\small}
    \renewcommand{\footnoterule}{\relax}
    \thispagestyle{empty}
%  \null\vfill
  \begin{center}
    { \Huge {\bfseries {\@title}} \par}
{\large \vspace*{35mm} {{\@crest} \par} \vspace*{25mm}}
    {{\Large \@author} \par}
	{\large \vspace*{2mm}}
    {{\large \@regcode} \par}
{\large \vspace*{2.5ex}
    {{\@collegeordept} \par}
\vspace*{0.5ex}
    {{\@university} \par}
\vspace*{5mm}
    {{\submittedtext} \par}
\vspace*{5mm}
    {\@degreedate}}

\large\vspace*{15mm}
	{{\@supervisor} \par}
    {{\@supervprof} \par}

  \end{center}
  \null\vfill
\end{alwayssingle}}

% Headings

% DEDICATION
%
% The dedication environment makes sure the dedication gets its
% own page and is set out in verse format.

\newenvironment{dedication}
{\begin{alwayssingle}
  \pagestyle{empty}
  \begin{center}
  \vspace*{1.5cm}
  {\LARGE }
  \end{center}
  \vspace{0.5cm}
  \begin{quote} \begin{center}}
{\end{center} \end{quote} \end{alwayssingle}}


% ACKNOWLEDGEMENTS
%
% The acknowledgements environment puts a large, bold, centered 
% "Acknowledgements" label at the top of the page. The acknowledgements
% themselves appear in a quote environment, i.e. tabbed in at both sides, and
% on its own page.

\newenvironment{acknowledgements}
{\pagestyle{empty}
\begin{alwayssingle}
\begin{center}
\vspace*{1.5cm}
{\Large \bfseries Acknowledgements}
\end{center}
\vspace{0.5cm}
\begin{quote}}
{\end{quote}\end{alwayssingle}}

% The acknowledgementslong environment puts a large, bold, centered 
% "Acknowledgements" label at the top of the page. The acknowledgement itself 
% does not appears in a quote environment so you can get more in.

\newenvironment{acknowledgementslong}
{\pagestyle{empty}
\begin{alwayssingle}
\begin{center}
\vspace*{1.5cm}
{\Large \bfseries Acknowledgements}
\end{center}
\vspace{0.5cm}\begin{quote}}
{\end{quote}\end{alwayssingle}}

%ROMANPAGES
%
% The romanpages environment set the page numbering to lowercase roman one
% for the contents and figures lists. It also resets
% page-numbering for the remainder of the dissertation (arabic, starting at 1).

\newenvironment{romanpages}
{\setcounter{page}{1}
  \renewcommand{\thepage}{\roman{page}}}
{\newpage\renewcommand{\thepage}{\arabic{page}}}
%{\newpage\renewcommand{\thepage}{\arabic{page}}\setcounter{page}{1}}
