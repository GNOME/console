/* kgx-local-page.c
 *
 * Copyright 2019 Zander Brown
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:kgx-local-page
 * @title: KgxLocalPage
 * @short_description: #KgxPage for an old fashioned local terminal
 * 
 * Since: 0.3.0
 */

#include <glib/gi18n.h>

#include "kgx-config.h"
#include "kgx-terminal.h"
#include "kgx-local-page.h"


G_DEFINE_TYPE (KgxLocalPage, kgx_local_page, KGX_TYPE_PAGE)


static void
kgx_local_page_class_init (KgxLocalPageClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               RES_PATH "kgx-local-page.ui");

  gtk_widget_class_bind_template_child (widget_class, KgxLocalPage, terminal);
}

static void
kgx_local_page_init (KgxLocalPage *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  
  kgx_page_connect_terminal (KGX_PAGE (self), KGX_TERMINAL (self->terminal));
}
