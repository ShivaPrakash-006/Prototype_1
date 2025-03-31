#include "sprite.h"
#include "includes.h"
#include "objects.h"

/* Sprite Definitions */
/* Struct */

SDL_FRect getSpriteRect(Sprite *spriteList, const char *spriteName) {
  SDL_FRect spriteRect = {0, 0, 0, 0};

  for (int i = 0; i < SPRITEMAX; i++)
    if (strncmp(spriteList[i].name, spriteName, 50) == 0) {
      spriteRect.x = spriteList[i].x;
      spriteRect.y = spriteList[i].y;
      spriteRect.w = spriteList[i].width;
      spriteRect.h = spriteList[i].height;
      break;
    }

  return spriteRect;
}

bool parseXML(const char *fileName, Sprite *spriteList) {
  bool success = false;
  xmlDoc *spriteXML = xmlReadFile(fileName, NULL, 0);
  if (spriteXML == NULL) {
    printf("'%s' could not be loaded!\n", fileName);
    success = false;
  }

  xmlNode *root = xmlDocGetRootElement(spriteXML);
  xmlNode *curNode = root->children;
  int spriteNum = 0;

  while (curNode != NULL) {
    if (curNode->type == XML_ELEMENT_NODE) {
      Sprite *sprite = &spriteList[spriteNum];

      xmlChar *name = xmlGetProp(curNode, (const xmlChar *)"name");
      xmlChar *x = xmlGetProp(curNode, (const xmlChar *)"x");
      xmlChar *y = xmlGetProp(curNode, (const xmlChar *)"y");
      xmlChar *width = xmlGetProp(curNode, (const xmlChar *)"width");
      xmlChar *height = xmlGetProp(curNode, (const xmlChar *)"height");

      snprintf(sprite->name, sizeof(sprite->name), "%s", name);
      sprite->x = atoi((char *)x);
      sprite->y = atoi((char *)y);
      sprite->width = atoi((char *)width);
      sprite->height = atoi((char *)height);

      xmlFree(name);
      xmlFree(x);
      xmlFree(y);
      xmlFree(width);
      xmlFree(height);

      spriteNum++;
    }
    curNode = curNode->next;
  }
  xmlFreeDoc(spriteXML);

  return success;
}
