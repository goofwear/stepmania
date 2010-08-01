/* XmlFile - Simple XML reading and writing. */

#ifndef XmlFile_H
#define XmlFile_H

#include <map>
struct DateTime;
class RageFileBasic;

typedef map<RString,RString> XAttrs;
class XNode;
typedef multimap<RString,XNode*> XNodes;

#define FOREACH_Attr( pNode, Var ) \
	for( XAttrs::iterator Var = (pNode)->m_attrs.begin(); \
		Var != (pNode)->m_attrs.end(); \
		++Var )

#define FOREACH_CONST_Attr( pNode, Var ) \
	for( XAttrs::const_iterator Var = (pNode)->m_attrs.begin(); \
		Var != (pNode)->m_attrs.end(); \
		++Var )

#define FOREACH_Child( pNode, Var ) \
	XNodes::iterator Var##Iter; \
	XNode *Var = NULL; \
	for( Var##Iter = (pNode)->m_childs.begin(); \
		Var = (Var##Iter != (pNode)->m_childs.end())? Var##Iter->second:NULL, \
		Var##Iter != (pNode)->m_childs.end(); \
		++Var##Iter )

#define FOREACH_CONST_Child( pNode, Var ) \
	XNodes::const_iterator Var##Iter; \
	const XNode *Var = NULL; \
	for( Var##Iter = (pNode)->m_childs.begin(); \
		Var = (Var##Iter != (pNode)->m_childs.end())? Var##Iter->second:NULL, \
		Var##Iter != (pNode)->m_childs.end(); \
		++Var##Iter )

enum PCODE
{
	PIE_PARSE_WELL_FORMED = 0,
	PIE_ALONE_NOT_CLOSED,
	PIE_NOT_CLOSED,
	PIE_NOT_NESTED,
	PIE_ATTR_NO_VALUE,
	PIE_READ_ERROR
};

// Parse info.
struct PARSEINFO
{
	bool		trim_value;			// [set] do trim when parse?
	bool		entity_value;		// [set] do convert from reference to entity? ( &lt; -> < )

	char*		xml;				// [get] xml source
	bool		error_occur;		// [get] is occurance of error?
	const char*	error_pointer;		// [get] error position of xml source
	PCODE		error_code;			// [get] error code
	RString		error_string;		// [get] error string

	PARSEINFO() { trim_value = true; entity_value = true; xml = NULL; error_occur = false; error_pointer = NULL; error_code = PIE_PARSE_WELL_FORMED; }
};

// display optional environment
struct DISP_OPT
{
	bool newline;			// newline when new tag
	bool reference_value;	// do convert from entity to reference ( < -> &lt; )
	RString stylesheet;		// empty string = no stylesheet
	bool write_tabs;		// if false, don't write tab indent characters

	int tab_base;			// internal usage
	DISP_OPT()
	{
		newline = true;
		reference_value = true;
		stylesheet = "";
		write_tabs = true;
		tab_base = 0;
	}
};

// XMLNode structure
class XNode
{
public:
	RString m_sName;	// a duplicate of the m_sName in the parent's map
	RString	m_sValue;
	XNodes	m_childs;		// child node
	XAttrs	m_attrs;		// attributes

	void GetValue( RString &out ) const;
	void GetValue( int &out ) const;
	void GetValue( float &out ) const;
	void GetValue( bool &out ) const;
	void GetValue( unsigned &out ) const;
	void GetValue( DateTime &out ) const;
	void SetValue( int v );
	void SetValue( float v );
	void SetValue( bool v );
	void SetValue( unsigned v );
	void SetValue( const DateTime &v );

	// Load/Save XML
	unsigned Load( const RString &sXml, PARSEINFO *pi, unsigned iOffset = 0 );
	unsigned LoadAttributes( const RString &sAttrs, PARSEINFO *pi, unsigned iOffset );
	bool GetXML( RageFileBasic &f, DISP_OPT &opt ) const;
	bool GetAttrXML( RageFileBasic &f, DISP_OPT &opt, const RString &sName, const RString &sValue ) const;
	RString GetXML() const;

	bool SaveToFile( const RString &sFile, DISP_OPT &opt ) const;
	bool SaveToFile( RageFileBasic &f, DISP_OPT &opt ) const;

	// in own attribute list
	const RString *GetAttr( const RString &sAttrName ) const; 
	RString *GetAttr( const RString &sAttrName ); 
	bool GetAttrValue( const RString &sName, RString &out ) const;
	bool GetAttrValue( const RString &sName, int &out ) const;
	bool GetAttrValue( const RString &sName, float &out ) const;
	bool GetAttrValue( const RString &sName, bool &out ) const;
	bool GetAttrValue( const RString &sName, unsigned &out ) const;
	bool GetAttrValue( const RString &sName, DateTime &out ) const;

	// in one level child nodes
	const XNode *GetChild( const RString &sName ) const; 
	XNode *GetChild( const RString &sName ); 
	bool GetChildValue( const RString &sName, RString &out ) const  { const XNode* pChild=GetChild(sName); if(pChild==NULL) return false; pChild->GetValue(out); return true; }
	bool GetChildValue( const RString &sName, int &out ) const      { const XNode* pChild=GetChild(sName); if(pChild==NULL) return false; pChild->GetValue(out); return true; }
	bool GetChildValue( const RString &sName, float &out ) const    { const XNode* pChild=GetChild(sName); if(pChild==NULL) return false; pChild->GetValue(out); return true; }
	bool GetChildValue( const RString &sName, bool &out ) const     { const XNode* pChild=GetChild(sName); if(pChild==NULL) return false; pChild->GetValue(out); return true; }
	bool GetChildValue( const RString &sName, unsigned &out ) const { const XNode* pChild=GetChild(sName); if(pChild==NULL) return false; pChild->GetValue(out); return true; }
	bool GetChildValue( const RString &sName, DateTime &out ) const { const XNode* pChild=GetChild(sName); if(pChild==NULL) return false; pChild->GetValue(out); return true; }

	// modify DOM 
	XNode *AppendChild( const RString &sName = RString(), const RString &value = RString() );
	XNode *AppendChild( const RString &sName, float value );
	XNode *AppendChild( const RString &sName, int value );
	XNode *AppendChild( const RString &sName, unsigned value );
	XNode *AppendChild( const RString &sName, const DateTime &value );
	XNode *AppendChild( XNode *node );
	bool RemoveChild( XNode *node );

	void AppendAttr( const RString &sName = RString(), const RString &sValue = RString() );
	void AppendAttr( const RString &sName, float value );
	void AppendAttr( const RString &sName, int value );
	void AppendAttr( const RString &sName, unsigned value );
	void AppendAttr( const RString &sName, const DateTime &value );
	bool RemoveAttr( const RString &sName );

	XNode() { }
	XNode( const XNode &cpy );
	~XNode();

	void Clear();
};

#endif
