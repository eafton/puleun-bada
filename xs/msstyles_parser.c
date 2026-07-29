#include "msstyles_parser.h"
#include <libpe/pe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_SIZEOF_SHORT_NAME 8

GHashTable *image_cache = NULL;

static gchar *msstyles_ini_normalize_comments(const gchar *text) {
    GString *out;
    const gchar *p;

    if (!text)
        return NULL;

    out = g_string_new(NULL);

    for (p = text; *p; p++) {
        if (*p == ';') {
            while (*p && *p != '\n')
                p++;
            if (!*p)
                break;
        }
        g_string_append_c(out, *p);
    }

    return g_string_free(out, FALSE);
}

gchar **msstyles_parse_utf16_string_list(const gchar *data, gsize size, gsize *out_count) {
    gchar **result;
    gchar *utf8;
    gsize bytes_read;
    gsize bytes_written;
    gsize count;
    gsize i;
    gsize j;

    if (out_count)
        *out_count = 0;

    if (!data || size < 2)
        return NULL;

    utf8 = g_convert(data, (gssize)size, "UTF-8", "UTF-16LE", &bytes_read, &bytes_written, NULL);
    if (!utf8)
        return NULL;

    for (i = 0; i < bytes_written; i++) {
        if (utf8[i] == '\r' || utf8[i] == '\n' || utf8[i] == '\0') {
            utf8[i] = '\0';
        }
    }

    count = 0;
    for (i = 0; i < bytes_written; i++) {
        if (utf8[i] != '\0' && (i == 0 || utf8[i - 1] == '\0')) {
            count++;
        }
    }

    if (count == 0) {
        g_free(utf8);
        return NULL;
    }

    result = g_new0(gchar *, count + 1);
    j = 0;
    for (i = 0; i < bytes_written && j < count; i++) {
        if (utf8[i] != '\0' && (i == 0 || utf8[i - 1] == '\0')) {
            result[j++] = g_strdup(&utf8[i]);
        }
    }

    g_free(utf8);

    if (out_count)
        *out_count = j;

    return result;
}

static gint msstyles_find_name_index(gchar **names, const gchar *wanted) {
    gsize i;

    if (!names || !wanted)
        return -1;

    for (i = 0; names[i]; i++) {
        if (g_ascii_strcasecmp(names[i], wanted) == 0)
            return (gint)i;
    }

    return -1;
}

static gint msstyles_resolve_size_index(gchar **size_names, const gchar *font_size) {
    static const struct {
        const gchar *rc_name;
        const gchar *msstyles_name;
    } aliases[] = {{"Normal", "NormalSize"}, {"LargeFonts", "LargeSize"}, {"ExtraLargeFonts", "ExtraLargeSize"}};
    gint idx;
    gsize i;

    if (!size_names)
        return 0;

    if (!font_size || !font_size[0])
        return 0;

    idx = msstyles_find_name_index(size_names, font_size);
    if (idx >= 0)
        return idx;

    for (i = 0; i < G_N_ELEMENTS(aliases); i++) {
        if (g_ascii_strcasecmp(font_size, aliases[i].rc_name) == 0) {
            idx = msstyles_find_name_index(size_names, aliases[i].msstyles_name);
            if (idx >= 0)
                return idx;
        }
    }

    return 0;
}

static gchar *msstyles_select_ini_resource(const gchar *msstyles_path, const gchar *color_scheme, const gchar *font_size) {
    gchar *color_data;
    gchar *size_data;
    gchar *file_data;
    gsize color_size;
    gsize size_size;
    gsize file_size;
    gchar **color_names;
    gchar **size_names;
    gchar **file_names;
    gsize color_count;
    gsize size_count;
    gsize file_count;
    gint color_idx;
    gint size_idx;
    gint resource_idx;
    gchar *ini_name;

    ini_name = NULL;
    color_names = NULL;
    size_names = NULL;
    file_names = NULL;

    color_data = msstyles_extract_resource_data(msstyles_path, "COLORNAMES", &color_size);
    size_data = msstyles_extract_resource_data(msstyles_path, "SIZENAMES", &size_size);
    file_data = msstyles_extract_resource_data(msstyles_path, "FILERESNAMES", &file_size);

    if (!color_data || !size_data || !file_data) {
        g_free(color_data);
        g_free(size_data);
        g_free(file_data);
        return g_strdup("NORMALBLUE_INI");
    }

    color_names = msstyles_parse_utf16_string_list(color_data, color_size, &color_count);
    size_names = msstyles_parse_utf16_string_list(size_data, size_size, &size_count);
    file_names = msstyles_parse_utf16_string_list(file_data, file_size, &file_count);

    g_free(color_data);
    g_free(size_data);
    g_free(file_data);

    if (!color_names || !size_names || !file_names || color_count == 0 || size_count == 0) {
        g_strfreev(color_names);
        g_strfreev(size_names);
        g_strfreev(file_names);
        return g_strdup("NORMALBLUE_INI");
    }

    if (color_scheme && color_scheme[0])
        color_idx = msstyles_find_name_index(color_names, color_scheme);
    else
        color_idx = -1;

    if (color_idx < 0)
        color_idx = 0;

    if (color_idx >= (gint)color_count)
        color_idx = color_count - 1;

    size_idx = msstyles_resolve_size_index(size_names, font_size);

    if (size_idx >= (gint)size_count)
        size_idx = size_count - 1;

    resource_idx = (gint)(size_count * (gsize)color_idx) + size_idx;
    if (resource_idx < 0 || (gsize)resource_idx >= file_count || !file_names[resource_idx])
        resource_idx = 0;

    ini_name = g_strdup(file_names[resource_idx]);

    g_strfreev(color_names);
    g_strfreev(size_names);
    g_strfreev(file_names);

    return ini_name;
}

GKeyFile *msstyles_load_theme_ini(const gchar *msstyles_path, const gchar *color_scheme, const gchar *font_size) {
    pe_ctx_t ctx;
    GKeyFile *key_file;
    GError *parse_error;
    gchar *ini_content;
    gchar *ini_resource;
    gsize ini_size;
    gboolean result;
    gchar *utf8_content;
    gchar *normalized_content;
    gsize bytes_read;
    gsize bytes_written;

    if (!msstyles_path)
        return NULL;

    if (pe_load_file(&ctx, msstyles_path) != LIBPE_E_OK) {
        return NULL;
    }

    if (pe_parse(&ctx) != LIBPE_E_OK) {
        pe_unload(&ctx);
        return NULL;
    }

    pe_unload(&ctx);

    ini_resource = msstyles_select_ini_resource(msstyles_path, color_scheme, font_size);
    if (!ini_resource)
        return NULL;

    ini_content = msstyles_extract_resource_data(msstyles_path, ini_resource, &ini_size);
    g_free(ini_resource);

    if (!ini_content || ini_size == 0) {
        g_free(ini_content);
        return NULL;
    }

    utf8_content = g_convert(ini_content, (gssize)ini_size, "UTF-8", "UTF-16LE", &bytes_read, &bytes_written, NULL);

    if (!utf8_content) {
        utf8_content = g_strndup(ini_content, ini_size);
    }

    parse_error = NULL;
    normalized_content = msstyles_ini_normalize_comments(utf8_content);
    key_file = g_key_file_new();
    result = g_key_file_load_from_data(key_file, normalized_content, -1, G_KEY_FILE_NONE, &parse_error);

    if (parse_error) {
        g_error_free(parse_error);
    }

    g_free(ini_content);
    g_free(utf8_content);
    g_free(normalized_content);

    if (!result) {
        g_key_file_unref(key_file);
        return NULL;
    }

    return key_file;
}

gchar *msstyles_find_section(GKeyFile *ini, const gchar *section) {
    gchar **groups;
    gchar *result = NULL;
    gsize i;

    if (!ini || !section)
        return NULL;

    groups = g_key_file_get_groups(ini, NULL);
    if (!groups)
        return NULL;

    for (i = 0; groups[i]; i++) {
        if (g_ascii_strcasecmp(groups[i], section) == 0) {
            result = g_strdup(groups[i]);
            break;
        }
    }

    g_strfreev(groups);
    return result;
}

const pe_resource_node_t *find_resource_node_by_name(const pe_resource_node_t *node, const char *name) {
    const pe_resource_node_t *child;
    const pe_resource_node_t *found;

    found = NULL;

    if (!node)
        return NULL;

    if (node->name && g_ascii_strcasecmp(node->name, name) == 0)
        return node;

    for (child = node->childNode; child; child = child->nextNode) {
        found = find_resource_node_by_name(child, name);
        if (found)
            return found;
    }

    return NULL;
}

const pe_resource_node_t *find_first_data_entry(const pe_resource_node_t *node) {
    const pe_resource_node_t *child;
    const pe_resource_node_t *found;

    found = NULL;

    if (!node)
        return NULL;

    if (node->type == LIBPE_RDT_DATA_ENTRY)
        return node;

    for (child = node->childNode; child; child = child->nextNode) {
        found = find_first_data_entry(child);
        if (found)
            return found;
    }
    return NULL;
}

gchar *msstyles_extract_resource_data(const gchar *msstyles_path, const gchar *resource_name, gsize *out_size) {
    pe_ctx_t ctx;
    pe_resources_t *res;
    const pe_resource_node_t *target_node = NULL;
    gchar *content = NULL;

    if (out_size)
        *out_size = 0;

    if (!msstyles_path || !resource_name)
        return NULL;

    if (pe_load_file(&ctx, msstyles_path) != LIBPE_E_OK)
        return NULL;

    if (pe_parse(&ctx) != LIBPE_E_OK) {
        pe_unload(&ctx);
        return NULL;
    }

    res = pe_resources(&ctx);
    if (res && res->root_node) {
        target_node = find_resource_node_by_name(res->root_node, resource_name);

        if (target_node) {
            const pe_resource_node_t *data_node = find_first_data_entry(target_node);

            if (data_node && data_node->type == LIBPE_RDT_DATA_ENTRY) {
                IMAGE_RESOURCE_DATA_ENTRY *entry = data_node->raw.dataEntry;
                if (entry) {
                    uint32_t rva = entry->OffsetToData;
                    uint32_t size = entry->Size;
                    uint64_t offset = pe_rva2ofs(&ctx, rva);

                    if (offset != 0) {
                        FILE *fp = fopen(msstyles_path, "rb");
                        if (fp) {
                            if (fseek(fp, (long)offset, SEEK_SET) == 0) {
                                content = g_malloc(size + 1);
                                if (fread(content, 1, size, fp) == size) {
                                    content[size] = '\0';
                                    if (out_size)
                                        *out_size = size;
                                } else {
                                    g_free(content);
                                    content = NULL;
                                }
                            }
                            fclose(fp);
                        }
                    }
                }
            }
        }
    }

    pe_unload(&ctx);
    return content;
}

gchar *msstyles_extract_resource(const gchar *msstyles_path, const gchar *resource_name) {
    return msstyles_extract_resource_data(msstyles_path, resource_name, NULL);
}

static gchar *msstyles_image_resource_name(const gchar *image_path) {
    gchar *result;
    gsize i, len;

    if (!image_path)
        return NULL;

    len = strlen(image_path);
    result = g_malloc(len + 1);

    for (i = 0; i < len; i++) {
        gchar c = image_path[i];
        if (g_ascii_isalnum(c))
            result[i] = g_ascii_toupper(c);
        else
            result[i] = '_';
    }
    result[len] = '\0';

    return result;
}

static guint32 msstyles_read_le32(const guchar *p) {
    return (guint32)p[0] | ((guint32)p[1] << 8) | ((guint32)p[2] << 16) | ((guint32)p[3] << 24);
}

static guint16 msstyles_read_le16(const guchar *p) {
    return (guint16)p[0] | ((guint16)p[1] << 8);
}

static gchar *msstyles_dib_to_bmp(const gchar *data, gsize size, gsize *out_size) {
    const guchar *bytes = (const guchar *)data;
    guint32 header_size;
    guint16 bit_count;
    guint32 compression;
    guint32 clr_used;
    guint32 palette_bytes;
    guint32 pixel_offset;
    guint32 file_size;
    gchar *result;
    guchar file_header[14];
    gboolean is_32bit_alpha;

    if (out_size)
        *out_size = 0;

    if (!data || size < 40)
        return NULL;

    if (bytes[0] == 'B' && bytes[1] == 'M') {
        result = g_malloc(size);
        memcpy(result, data, size);
        if (out_size)
            *out_size = size;
        return result;
    }

    header_size = msstyles_read_le32(bytes);
    bit_count = msstyles_read_le16(bytes + 14);
    compression = msstyles_read_le32(bytes + 16);
    clr_used = msstyles_read_le32(bytes + 32);

    is_32bit_alpha = (bit_count == 32 && header_size == 40 && compression == 0);

    palette_bytes = 0;
    if (bit_count > 0 && bit_count <= 8) {
        guint32 num_colors = clr_used ? clr_used : ((guint32)1 << bit_count);
        palette_bytes = num_colors * 4;
    }

    if (is_32bit_alpha) {
        pixel_offset = 14 + 56;
        file_size = pixel_offset + ((guint32)size - header_size);
    } else {
        guint32 extra_bitfields = (compression == 3 && header_size == 40) ? 12 : 0;
        pixel_offset = 14 + header_size + extra_bitfields + palette_bytes;
        file_size = 14 + (guint32)size;
    }

    file_header[0] = 'B';
    file_header[1] = 'M';
    file_header[2] = (guchar)(file_size & 0xff);
    file_header[3] = (guchar)((file_size >> 8) & 0xff);
    file_header[4] = (guchar)((file_size >> 16) & 0xff);
    file_header[5] = (guchar)((file_size >> 24) & 0xff);
    file_header[6] = 0;
    file_header[7] = 0;
    file_header[8] = 0;
    file_header[9] = 0;
    file_header[10] = (guchar)(pixel_offset & 0xff);
    file_header[11] = (guchar)((pixel_offset >> 8) & 0xff);
    file_header[12] = (guchar)((pixel_offset >> 16) & 0xff);
    file_header[13] = (guchar)((pixel_offset >> 24) & 0xff);

    if (is_32bit_alpha) {
        result = g_malloc(file_size);
        memcpy(result, file_header, 14);

        result[14] = 56;
        result[15] = 0;
        result[16] = 0;
        result[17] = 0;

        memcpy(result + 18, bytes + 4, 12);

        result[30] = 3;
        result[31] = 0;
        result[32] = 0;
        result[33] = 0;

        memcpy(result + 34, bytes + 20, 20);

        result[54] = 0xFF;
        result[55] = 0;
        result[56] = 0;
        result[57] = 0;

        result[58] = 0;
        result[59] = 0xFF;
        result[60] = 0;
        result[61] = 0;

        result[62] = 0;
        result[63] = 0;
        result[64] = 0xFF;
        result[65] = 0;

        result[66] = 0;
        result[67] = 0;
        result[68] = 0;
        result[69] = 0xFF;

        memcpy(result + 70, bytes + 40, size - 40);
    } else {
        result = g_malloc(file_size);
        memcpy(result, file_header, 14);
        memcpy(result + 14, data, size);
    }

    if (out_size)
        *out_size = file_size;

    return result;
}

GdkPixbuf *msstyles_load_image(const gchar *msstyles_path, const gchar *image_name) {
    GdkPixbufLoader *loader;
    GdkPixbuf *pixbuf;
    GError *error;
    gchar *resource_name;
    gchar *content;
    gchar *bmp_data;
    gsize size;
    gsize bmp_size;

    pixbuf = NULL;
    error = NULL;

    if (!image_cache) {
        image_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    }

    pixbuf = g_hash_table_lookup(image_cache, image_name);
    if (pixbuf) {
        g_object_ref(pixbuf);
        return pixbuf;
    }

    resource_name = msstyles_image_resource_name(image_name);
    if (!resource_name)
        return NULL;

    content = msstyles_extract_resource_data(msstyles_path, resource_name, &size);
    g_free(resource_name);
    if (!content)
        return NULL;

    bmp_data = msstyles_dib_to_bmp(content, size, &bmp_size);
    g_free(content);

    if (!bmp_data)
        return NULL;

    loader = gdk_pixbuf_loader_new();
    if (gdk_pixbuf_loader_write(loader, (const guchar *)bmp_data, bmp_size, &error)) {
        gdk_pixbuf_loader_close(loader, NULL);
        pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
        if (pixbuf) {
            if (!gdk_pixbuf_get_has_alpha(pixbuf)) {
                GdkPixbuf *alpha_pixbuf;
                alpha_pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
                g_object_unref(pixbuf);
                pixbuf = alpha_pixbuf;
            }
            g_object_ref(pixbuf);
            g_hash_table_insert(image_cache, g_strdup(image_name), pixbuf);
            g_object_ref(pixbuf);
        }
    } else {
        g_error_free(error);
    }

    g_object_unref(loader);
    g_free(bmp_data);

    return pixbuf;
}

gboolean msstyles_parse_color(GKeyFile *ini, const gchar *section, const gchar *key, GdkColor *color) {
    gchar *color_str;
    gint r, g, b;
    gboolean result;

    if (!ini || !section || !key || !color)
        return FALSE;

    color_str = g_key_file_get_string(ini, section, key, NULL);
    if (!color_str)
        return FALSE;

    result = sscanf(color_str, "%d %d %d", &r, &g, &b) == 3;
    if (result) {
        color->red = r * 257;
        color->green = g * 257;
        color->blue = b * 257;
    }

    g_free(color_str);
    return result;
}

gboolean msstyles_get_sys_color(GKeyFile *ini, const gchar *name, GdkColor *out) {
    if (!ini || !name || !out)
        return FALSE;

    return msstyles_parse_color(ini, "SysMetrics", name, out);
}

gchar *msstyles_get_image_path(GKeyFile *ini, const gchar *section, const gchar *state) {
    gchar *image_key;
    gchar *image_path;

    if (!ini || !section || !state)
        return NULL;

    image_key = g_strdup_printf("ImageFile.%s", state);
    image_path = g_key_file_get_string(ini, section, image_key, NULL);
    g_free(image_key);

    if (!image_path) {
        image_path = g_key_file_get_string(ini, section, "ImageFile", NULL);
    }

    return image_path;
}

gboolean msstyles_parse_margins(GKeyFile *ini, const gchar *section, const gchar *key, GtkBorder *margins) {
    gchar *margin_str;
    gchar **tokens;
    gboolean result;
    gint vals[4];
    gint count;
    gint i;

    result = FALSE;
    vals[0] = 0;
    vals[1] = 0;
    vals[2] = 0;
    vals[3] = 0;
    count = 0;

    if (!ini || !section || !key || !margins)
        return FALSE;

    margin_str = g_key_file_get_string(ini, section, key, NULL);
    if (!margin_str)
        return FALSE;

    tokens = g_strsplit_set(margin_str, " ,", -1);
    if (tokens) {
        for (i = 0; tokens[i] && count < 4; i++) {
            if (tokens[i][0] != '\0') {
                vals[count++] = (gint)g_ascii_strtoll(tokens[i], NULL, 10);
            }
        }

        if (count == 4) {
            margins->left = vals[0];
            margins->right = vals[1];
            margins->top = vals[2];
            margins->bottom = vals[3];
            result = TRUE;
        } else if (count == 2) {
            margins->left = vals[0];
            margins->right = vals[0];
            margins->top = vals[1];
            margins->bottom = vals[1];
            result = TRUE;
        } else if (count == 1) {
            margins->left = vals[0];
            margins->right = vals[0];
            margins->top = vals[0];
            margins->bottom = vals[0];
            result = TRUE;
        }

        if (result) {
            if (margins->left < 0)
                margins->left = 0;
            if (margins->right < 0)
                margins->right = 0;
            if (margins->top < 0)
                margins->top = 0;
            if (margins->bottom < 0)
                margins->bottom = 0;
        }
        g_strfreev(tokens);
    }

    g_free(margin_str);
    return result;
}

gint msstyles_parse_int(GKeyFile *ini, const gchar *section, const gchar *key, gint default_value) {
    gint val = default_value;
    GError *error;

    error = NULL;

    if (!ini || !section || !key)
        return default_value;

    val = g_key_file_get_integer(ini, section, key, &error);
    if (error) {
        val = default_value;
        g_error_free(error);
    }

    return val;
}
