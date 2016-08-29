/***************************************************************************
* Copyright (C) 2015-2016 Alexander V. Popov.
* 
* This file is part of Tightly Associated Solvent Shell Extractor (TASSE) 
* source code.
* 
* TASSE is free software; you can redistribute it and/or modify it under 
* the terms of the GNU General Public License as published by the Free 
* Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
* 
* TASSE is distributed in the hope that it will be useful, but WITHOUT 
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.
* 
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
***************************************************************************/
#ifndef TASSE_VALUE_FOR_CONTROL_H
#define TASSE_VALUE_FOR_CONTROL_H

#include <QtCore/QString>

typedef enum {
	CTRL_CHECKBOX = 0,
	CTRL_SLIDER,
	CTRL_INPUT,
	CTRL_FILE_PDB,
	CTRL_FILE_TRJ,
	CTRL_FILE_ANY
} ctrlType_e;

typedef enum {
	CVAR_CHARS = 0,
	CVAR_SIZET,
	CVAR_REAL,
	CVAR_BOOL,
	CVAR_INT
} cvarType_e;

class QWidget;

struct LayoutControlDesc {
	const char	*title;
	ctrlType_e	ctrlType;
	cvarType_e	cvarType;
	real		minValue;
	real		maxValue;
	void		*value;
	size_t		valueSize;
	mutable QWidget	*w_ptr;
};

#define DEFINE_CONTROL( title, ctrlType, cvarType, minV, maxV, Vptr ) \
	{ title, ctrlType, cvarType, minV, maxV, Vptr, sizeof(Vptr), nullptr }

template<typename T>
class ValueForControl
{
public:
	explicit ValueForControl( const struct LayoutControlDesc *ctrl ) {
		assert( ctrl != nullptr );
		ctrl_ = ctrl;
	}
	operator T() const {
		assert( ctrl_ != nullptr );
		switch( ctrl_->cvarType ) {
		case CVAR_CHARS:
			return static_cast<T>( atof( reinterpret_cast<const char*>( ctrl_->value ) ) );
		case CVAR_INT:
			return static_cast<T>( *reinterpret_cast<int*>( ctrl_->value ) );
		case CVAR_SIZET:
			return static_cast<T>( *reinterpret_cast<size_t*>( ctrl_->value ) );
		case CVAR_REAL:
			return static_cast<T>( *reinterpret_cast<real*>( ctrl_->value ) );
		case CVAR_BOOL:
			return *reinterpret_cast<bool*>( ctrl_->value ) ? T( 1 ) : T( 0 );
		default:
			return T();
		}
	}
	void assign( const T& value )
	{
		assert( ctrl_ != nullptr );
		switch( ctrl_->cvarType ) {
		case CVAR_CHARS:
			sprintf_s( reinterpret_cast<char*>( ctrl_->value ), ctrl_->valueSize, "%g", static_cast<float>( value ) );
			break;
		case CVAR_INT:
			*reinterpret_cast<int*>( ctrl_->value ) = value;
			break;
		case CVAR_SIZET:
			*reinterpret_cast<size_t*>( ctrl_->value ) = value;
			break;
		case CVAR_REAL:
			*reinterpret_cast<real*>( ctrl_->value ) = value;
			break;
		case CVAR_BOOL:
			*reinterpret_cast<bool*>( ctrl_->value ) = value;
			break;
		default:
			break;
		}
	}
private:
	const struct LayoutControlDesc *ctrl_;
};

template<>
class ValueForControl<QString>
{
public:
	explicit ValueForControl( const struct LayoutControlDesc *ctrl ) {
		assert( ctrl != nullptr );
		ctrl_ = ctrl;
	}
	operator QString() const {
		assert( ctrl_ != nullptr );
		switch( ctrl_->cvarType ) {
		case CVAR_CHARS:
			return QString( reinterpret_cast<const char*>( ctrl_->value ) );
		case CVAR_INT:
			return QString::number( *reinterpret_cast<int*>( ctrl_->value ) );
		case CVAR_SIZET:
			return QString::number( *reinterpret_cast<size_t*>( ctrl_->value ) );
		case CVAR_REAL:
			return QString::number( *reinterpret_cast<real*>( ctrl_->value ) );
		case CVAR_BOOL:
			return *reinterpret_cast<bool*>( ctrl_->value ) ? QString( "true" ) : QString( "false" );
		default:
			return QString();
		}
	}
	void assign( const QString& value )
	{
		assert( ctrl_ != nullptr );
		switch( ctrl_->cvarType ) {
		case CVAR_CHARS:
			memset( ctrl_->value, 0, ctrl_->valueSize );
			strcat_s( reinterpret_cast<char*>( ctrl_->value ), ctrl_->valueSize, value.toLocal8Bit().data() );
			break;
		case CVAR_INT:
			*reinterpret_cast<int*>( ctrl_->value ) = value.toInt();
			break;
		case CVAR_SIZET:
			*reinterpret_cast<size_t*>( ctrl_->value ) = value.toULong();
			break;
		case CVAR_REAL:
			*reinterpret_cast<real*>( ctrl_->value ) = static_cast<real>( value.toDouble() );
			break;
		case CVAR_BOOL:
			*reinterpret_cast<bool*>( ctrl_->value ) = !!value.toInt();
			break;
		default:
			break;
		}
	}
private:
	const struct LayoutControlDesc *ctrl_;
};

#endif //TASSE_VALUE_FOR_CONTROL_H
